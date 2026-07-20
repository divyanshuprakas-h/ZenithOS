#include "page_table.h"

#include "../mm/pmm.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"


static uint64_t hhdm_offset;

static page_table_t *get_next_table(
    page_table_t *table,
    uint16_t index,
    bool create
)
{
    page_entry_t entry = table->entries[index];

    if (entry & PAGE_PRESENT)
    {
        uint64_t physical = entry & 0x000FFFFFFFFFF000ULL;

        return (page_table_t *)page_table_physical_to_virtual(physical);
    }

    if (!create)
    {
        return NULL;
    }

    void *physical_page = pmm_alloc_page();

    if (physical_page == NULL)
    {
        return NULL;
    }

    page_table_t *new_table = (page_table_t *)page_table_physical_to_virtual(
        (uint64_t)(uintptr_t)physical_page
    );

    k_memset(new_table, 0, PAGE_SIZE);

    table->entries[index] = ((uint64_t)(uintptr_t)physical_page) | PAGE_PRESENT | PAGE_WRITABLE ;

    return new_table;

}

static inline void invalidate_page(uint64_t virtual_address)
{
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"((void *)virtual_address)
        : "memory"
    );
}

static uint64_t read_cr3(void)
{
    uint64_t cr3;

    __asm__ volatile(
        "mov %%cr3, %0"
        : "=r"(cr3)
    );

    return cr3;
}

static void write_cr3(uint64_t cr3)
{
    __asm__ volatile (
        "mov %0, %%cr3"
        : 
        : "r"(cr3)
        : "memory"
    );

}

static page_table_t *kernel_pml4 = NULL;
static uint64_t kernel_pml4_physical = 0;

void page_table_set_hhdm_offset(uint64_t offset)
{
    hhdm_offset = offset;
}

void *page_table_physical_to_virtual(uint64_t physical_address)
{
    return (void *)(uintptr_t)(physical_address + hhdm_offset);
}

page_table_t *page_table_get_pml4(void)
{
    return kernel_pml4;
}

void page_table_init(void)
{
    void *pml4_physical = pmm_alloc_page();

    if (!pml4_physical)
    {
        kprintf("Failed to allocate PML4\n");
        return;
    }

    kernel_pml4_physical = (uint64_t)(uintptr_t)pml4_physical;

    kernel_pml4 = (page_table_t *)page_table_physical_to_virtual(
        kernel_pml4_physical
    );

    kprintf("Kernel PML4 PA : %p\n", (void *)kernel_pml4_physical);

    uint64_t current_cr3 = read_cr3();
    kprintf("Current CR3      : %p\n", (void *)current_cr3);

    page_table_t *current_pml4 = (page_table_t *)page_table_physical_to_virtual(current_cr3);
    kprintf("Current PML4 VA  : %p\n", current_pml4);

    k_memcpy(kernel_pml4, current_pml4, PAGE_SIZE);

    // For no mismatch (nothing prints)

    // for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    // {
    //     if (current_pml4->entries[i] != kernel_pml4->entries[i])
    //     {
    //         kprintf("Mismatch at entry %d\n", i);
    //     }
    // }

    // For non-zero test

    // for (int i = 0; i < PAGE_TABLE_ENTRIES; i++)
    // {
    //     if (kernel_pml4->entries[i] != 0)
    //     {
    //         kprintf("PML4[%d]\n", i, (void *)kernel_pml4->entries[i]);
    //     }
    // }

    kprintf("Current PML4[0] : %p\n", (void *)current_pml4->entries[0]);
    kprintf("Kernel PML4[0] : %p\n", (void *)kernel_pml4->entries[0]);

    kprintf("Kernel PML4  : %p\n", kernel_pml4);

}

bool page_table_map(uint64_t virtual_address, uint64_t physical_address, uint64_t flags)
{
    page_table_t *pml4 = kernel_pml4;
    
    page_table_t *pdpt = get_next_table(pml4, PML4_INDEX(virtual_address), true);

    if (pdpt == NULL)
    {
        return false;
    }

    page_table_t *pd = get_next_table(pdpt, PDPT_INDEX(virtual_address), true);

    if (pd == NULL)
    {
        return false;
    }

    page_table_t *pt = get_next_table(pd, PD_INDEX(virtual_address), true);

    if(pt == NULL)
    {
        return false;
    }

    uint16_t index = PT_INDEX(virtual_address);

    pt->entries[index] = (physical_address & 0x000FFFFFFFFFF000ULL) | flags | PAGE_PRESENT;

    return true;

}

bool page_table_unmap(uint64_t virtual_address)
{
    page_table_t *pml4 = kernel_pml4;

    page_table_t *pdpt = get_next_table(pml4, PML4_INDEX(virtual_address), false);

    if (pdpt == NULL)
    {
        return false;
    }

    page_table_t *pd = get_next_table(pdpt, PDPT_INDEX(virtual_address), false);

    if (pd == NULL)
    {
        return false;
    }

    page_table_t *pt = get_next_table(pt, PT_INDEX(virtual_address), false);

    if (pt == NULL)
    {
        return false;
    }

    pt->entries[PT_INDEX(virtual_address)] = 0;

    invalidate_page(virtual_address);

    return true;

}

uint64_t page_table_translate(uint64_t virtual_address)
{
    page_table_t *pml4 = kernel_pml4;

    page_table_t *pdpt = get_next_table(pml4, PML4_INDEX(virtual_address), false);

    if (pdpt == NULL)
    {
        return 0;
    }

    page_table_t *pd = get_next_table(pdpt, PDPT_INDEX(virtual_address), false);

    if (pd == NULL)
    {
        return 0;
    }

    page_table_t *pt = get_next_table(pd, PD_INDEX(virtual_address), false);

    if (pt == NULL)
    {
        return 0;
    }

    page_entry_t entry = pt->entries[PT_INDEX(virtual_address)];

    if (!entry & PAGE_PRESENT)
    {
        return 0;
    }

    uint64_t physical = entry & 0x000FFFFFFFFFF000ULL;

    return physical | PAGE_OFFSET(virtual_address);
}

void page_table_activate(void)
{
    kprintf("Switching to ZenithOS page tables...\n");

    write_cr3(kernel_pml4_physical);

    kprintf("CR3 switched successfully!\n");
}
