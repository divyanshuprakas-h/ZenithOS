#include "pmm.h"
#include "bitmap.h"
#include "../terminal/terminal.h"

#include "../stdio/printf.h"

#include <stddef.h>

#define PMM_PAGE_SIZE 4096ULL
#define PMM_BITMAP_MAX_SIZE (128 * 1024)
#define PMM_BITMAP_MAX_BITS (PMM_BITMAP_MAX_SIZE * 8ULL)

extern char _kernel_start[];
extern char _kernel_end[];

static uint64_t total_memory;
static uint64_t usable_memory;
static uint64_t reserved_memory;
static uint64_t highest_address;
static uint64_t highest_physical_address;

static uint64_t bitmap_size;

static uint64_t total_pages;
static uint64_t free_pages;
static uint64_t used_pages;
static uint64_t next_free_page;

static uint8_t kernel_bitmap[PMM_BITMAP_MAX_SIZE];

static uint64_t pmm_address_to_page(void *address)
{
    return (uint64_t)(uintptr_t)address / PMM_PAGE_SIZE;
}

static uint64_t pmm_region_page_count(uint64_t length)
{
    if (length == 0)
        return 0;

    return (length + PMM_PAGE_SIZE - 1ULL) / PMM_PAGE_SIZE;
}

static uint64_t pmm_clamp_page_count(uint64_t start_page, uint64_t page_count)
{
    if (start_page >= total_pages)
        return 0;

    uint64_t remaining = total_pages - start_page;

    if (page_count > remaining)
        return remaining;

    return page_count;
}

static void pmm_mark_page_used(uint64_t page)
{
    if (page >= total_pages)
        return;

    if (bitmap_test(page))
        return;

    bitmap_set(page);
    if (free_pages > 0)
        free_pages--;
    used_pages++;
}

static void pmm_mark_page_free(uint64_t page)
{
    if (page == 0 || page >= total_pages)
        return;

    if (!bitmap_test(page))
        return;

    bitmap_clear(page);
    free_pages++;
    if (used_pages > 0)
        used_pages--;

    if (page < next_free_page)
        next_free_page = page;
}

void pmm_reserve_region(uint64_t base, uint64_t length)
{
    uint64_t start_page = base / PMM_PAGE_SIZE;
    uint64_t page_count = pmm_clamp_page_count(
        start_page,
        pmm_region_page_count(length)
    );

    for (uint64_t page = start_page; page < start_page + page_count; page++)
    {
        pmm_mark_page_used(page);
    }
}

void pmm_unreserve_region(uint64_t base, uint64_t length)
{
    uint64_t start_page = base / PMM_PAGE_SIZE;
    uint64_t page_count = pmm_clamp_page_count(
        start_page,
        pmm_region_page_count(length)
    );

    for (uint64_t page = start_page; page < start_page + page_count; page++)
    {
        pmm_mark_page_free(page);
    }
}

static void pmm_reserve_memmap_type(
    volatile struct limine_memmap_response *response,
    uint64_t type
)
{
    for (uint64_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];

        if (entry->type != type)
            continue;

        pmm_reserve_region(entry->base, entry->length);
    }
}

static void pmm_mark_usable(
    volatile struct limine_memmap_response *response
)
{
    for (uint64_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        uint64_t start_page = entry->base / PMM_PAGE_SIZE;
        uint64_t page_count = pmm_clamp_page_count(
            start_page,
            pmm_region_page_count(entry->length)
        );

        for (uint64_t page = start_page; page < start_page + page_count; page++)
        {
            pmm_mark_page_free(page);
        }
    }
}

static void pmm_seek_next_free_page(void)
{
    while (next_free_page < total_pages && bitmap_test(next_free_page))
    {
        next_free_page++;
    }
}

void pmm_lock_page(void *address)
{
    pmm_mark_page_used(pmm_address_to_page(address));
}

void pmm_unlock_page(void *address)
{
    pmm_mark_page_free(pmm_address_to_page(address));
}

void pmm_unlock_pages(void *address, uint64_t count)
{
    uint64_t page = pmm_address_to_page(address);
    uint64_t page_count = pmm_clamp_page_count(page, count);

    for (uint64_t i = 0; i < page_count; i++)
    {
        pmm_mark_page_free(page + i);
    }
}

void pmm_lock_pages(void *address, uint64_t count)
{
    uint64_t page = pmm_address_to_page(address);
    uint64_t page_count = pmm_clamp_page_count(page, count);

    for (uint64_t i = 0; i < page_count; i++)
    {
        pmm_mark_page_used(page + i);
    }
}

void pmm_reserve_page(void *address)
{
    pmm_lock_page(address);
}

void pmm_unreserve_page(void *address)
{
    pmm_unlock_page(address);
}

void *pmm_alloc_page(void)
{
    if (free_pages == 0)
        return NULL;

    uint64_t start_page = next_free_page;

    for (uint64_t pass = 0; pass < 2; pass++)
    {
        uint64_t begin = pass == 0 ? start_page : 0;
        uint64_t end = pass == 0 ? total_pages : start_page;

        for (uint64_t page = begin; page < end; page++)
        {
            if (bitmap_test(page))
                continue;

            pmm_mark_page_used(page);

            next_free_page = page + 1;
            if (next_free_page >= total_pages)
                next_free_page = 0;

            pmm_seek_next_free_page();

            return (void *)(uintptr_t)(page * PMM_PAGE_SIZE);
        }
    }

    return NULL;
}

void pmm_free_page(void *address)
{
    pmm_unlock_page(address);
}

uint64_t pmm_get_free_pages(void)
{
    return free_pages;
}

uint64_t pmm_get_used_pages(void)
{
    return used_pages;
}

uint64_t pmm_get_total_pages(void)
{
    return total_pages;
}

void pmm_init(
    volatile struct limine_memmap_response *response,
    volatile struct limine_executable_address_response *executable_response
)
{
    total_memory = 0;
    usable_memory = 0;
    reserved_memory = 0;
    highest_address = 0;
    highest_physical_address = 0;

    free_pages = 0;
    used_pages = 0;
    next_free_page = 0;

    for (uint64_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];
        uint64_t end = entry->base + entry->length;

        total_memory += entry->length;

        if (end > highest_physical_address)
        {
            highest_physical_address = end;
        }

        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            usable_memory += entry->length;

            if (end > highest_address)
            {
                highest_address = end;
            }
        }
        else
        {
            reserved_memory += entry->length;
        }
    }

    total_pages = (highest_address + PMM_PAGE_SIZE - 1ULL) / PMM_PAGE_SIZE;
    bitmap_size = (total_pages + 7ULL) / 8ULL;

    if (bitmap_size > PMM_BITMAP_MAX_SIZE || total_pages > PMM_BITMAP_MAX_BITS)
    {
        kprintf("PMM bitmap too small for this memory map\n");
        return;
    }

    bitmap_init(kernel_bitmap, total_pages);

    pmm_mark_usable(response);

    pmm_reserve_region(0, PMM_PAGE_SIZE);

    if (executable_response != NULL)
    {
        uint64_t kernel_physical_base = executable_response->physical_base;
        uint64_t kernel_size = (uint64_t)((uintptr_t)_kernel_end -
            (uintptr_t)_kernel_start);

        pmm_reserve_region(kernel_physical_base, kernel_size);

        kprintf("Kernel Physical : %p\n", (void *)kernel_physical_base);
        kprintf("Kernel Size     : %u bytes\n", (unsigned)kernel_size);
    }

    pmm_reserve_memmap_type(response, LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE);
    pmm_reserve_memmap_type(response, LIMINE_MEMMAP_EXECUTABLE_AND_MODULES);
    pmm_reserve_memmap_type(response, LIMINE_MEMMAP_FRAMEBUFFER);

    pmm_seek_next_free_page();

    kprintf("Kernel Start : %p\n", _kernel_start);
    kprintf("Kernel End   : %p\n", _kernel_end);

    kprintf("\n");
    kprintf("========== Physical Memory ==========\n");

    kprintf("Total RAM      : %u MB\n", (unsigned)(total_memory / 1024 / 1024));
    kprintf("Usable RAM     : %u MB\n", (unsigned)(usable_memory / 1024 / 1024));
    kprintf("Reserved RAM   : %u MB\n", (unsigned)(reserved_memory / 1024 / 1024));
    kprintf("Highest Usable : %p\n", (void *)highest_address);
    kprintf("Highest Phys   : %p\n", (void *)highest_physical_address);
    kprintf("Total Pages    : %u\n", (unsigned)total_pages);
    kprintf("Bitmap Size    : %u bytes\n", (unsigned)bitmap_size);
    kprintf("Bitmap Buffer : Kernel Static\n");
    kprintf("Free Pages : %u\n", (unsigned)free_pages);
    kprintf("Used Pages : %u\n", (unsigned)used_pages);
    kprintf("=====================================\n");
}
