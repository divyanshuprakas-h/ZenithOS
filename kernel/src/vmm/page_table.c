#include "page_table.h"

#include "../mm/pmm.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"

typedef enum
{
    PAGE_WALK_LEVEL_NONE = 0,
    PAGE_WALK_LEVEL_PT,
    PAGE_WALK_LEVEL_PD,
    PAGE_WALK_LEVEL_PDPT,
} page_walk_level_t;

typedef enum
{
    PAGE_WALK_OK = 0,
    PAGE_WALK_INVALID_ROOT,
    PAGE_WALK_MISSING_PML4_ENTRY,
    PAGE_WALK_MISSING_PDPT_ENTRY,
    PAGE_WALK_MISSING_PD_ENTRY,
    PAGE_WALK_MISSING_PT_ENTRY,
    PAGE_WALK_HUGE_PAGE_PDPT,
    PAGE_WALK_HUGE_PAGE_PD,
    PAGE_WALK_ALLOCATION_FAILED,
    PAGE_WALK_HHDM_UNCONFIGURED,
} page_walk_status_t;

static uint64_t hhdm_offset;
static bool hhdm_ready = false;

static uint64_t active_pml4_physical = 0;

static page_table_t *ensure_next_table(
    page_table_t *table,
    uint16_t index,
    bool create,
    page_walk_status_t missing_status,
    page_walk_status_t *status
)
{
    if (table == NULL)
    {
        if (status)
        {
            *status = PAGE_WALK_INVALID_ROOT;
        }

        return NULL;
    }

    page_entry_t entry = table->entries[index];

    if (entry & PAGE_PRESENT)
    {
        uint64_t physical = entry & PAGE_TABLE_ADDRESS_MASK;

        return (page_table_t *)page_table_physical_to_virtual(physical);
    }

    if (!create)
    {
        if (status)
        {
            *status = missing_status;
        }

        return NULL;
    }

    void *physical_page = pmm_alloc_page();

    if (physical_page == NULL)
    {
        if (status)
        {
            *status = PAGE_WALK_ALLOCATION_FAILED;
        }

        return NULL;
    }

    page_table_t *new_table = (page_table_t *)page_table_physical_to_virtual(
        (uint64_t)(uintptr_t)physical_page
    );

    if (new_table == NULL)
    {
        pmm_free_page(physical_page);

        if (status)
        {
            *status = PAGE_WALK_HHDM_UNCONFIGURED;
        }

        return NULL;
    }

    k_memset(new_table, 0, PAGE_SIZE);

    table->entries[index] = ((uint64_t)(uintptr_t)physical_page) | PAGE_PRESENT | PAGE_WRITABLE;

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

static const char *page_walk_level_name(page_walk_level_t level)
{
    switch (level)
    {
        case PAGE_WALK_LEVEL_PT:
            return "PT (4 KiB)";
        case PAGE_WALK_LEVEL_PD:
            return "PD (2 MiB)";
        case PAGE_WALK_LEVEL_PDPT:
            return "PDPT (1 GiB)";
        default:
            return "Unknown";
    }
}

static const char *page_walk_status_name(page_walk_status_t status)
{
    switch (status)
    {
        case PAGE_WALK_OK:
            return "OK";
        case PAGE_WALK_INVALID_ROOT:
            return "Invalid root page table";
        case PAGE_WALK_MISSING_PML4_ENTRY:
            return "PML4 entry not present";
        case PAGE_WALK_MISSING_PDPT_ENTRY:
            return "PDPT entry not present";
        case PAGE_WALK_MISSING_PD_ENTRY:
            return "PD entry not present";
        case PAGE_WALK_MISSING_PT_ENTRY:
            return "PT entry not present";
        case PAGE_WALK_HUGE_PAGE_PDPT:
            return "Hit 1 GiB large page";
        case PAGE_WALK_HUGE_PAGE_PD:
            return "Hit 2 MiB large page";
        case PAGE_WALK_ALLOCATION_FAILED:
            return "Failed to allocate a page table";
        case PAGE_WALK_HHDM_UNCONFIGURED:
            return "HHDM is not configured";
        default:
            return "Unknown status";
    }
}

static inline uint64_t page_size_for_level(page_walk_level_t level)
{
    switch (level)
    {
        case PAGE_WALK_LEVEL_PT:
            return PAGE_SIZE;
        case PAGE_WALK_LEVEL_PD:
            return PAGE_2MIB_SIZE;
        case PAGE_WALK_LEVEL_PDPT:
            return PAGE_1GIB_SIZE;
        default:
            return 0;
    }
}

static page_table_t *page_table_current_root(void)
{
    if (!hhdm_ready)
    {
        return NULL;
    }

    uint64_t current_cr3 = read_cr3() & PAGE_TABLE_ADDRESS_MASK;

    return (page_table_t *)page_table_physical_to_virtual(current_cr3);
}

static page_table_t *page_table_active_root(void)
{
    if (kernel_pml4 != NULL)
    {
        return kernel_pml4;
    }

    return page_table_current_root();
}

static page_entry_t *page_walk_internal(
    page_table_t *pml4,
    uint64_t virtual_address,
    bool create,
    page_walk_level_t *level_out,
    page_walk_status_t *status_out
)
{
    if (level_out)
    {
        *level_out = PAGE_WALK_LEVEL_NONE;
    }

    if (status_out)
    {
        *status_out = PAGE_WALK_OK;
    }

    if (pml4 == NULL)
    {
        if (status_out)
        {
            *status_out = PAGE_WALK_INVALID_ROOT;
        }

        return NULL;
    }

    page_table_t *pdpt = ensure_next_table(
        pml4,
        PML4_INDEX(virtual_address),
        create,
        PAGE_WALK_MISSING_PML4_ENTRY,
        status_out
    );

    if (pdpt == NULL)
    {
        return NULL;
    }

    page_entry_t *pdpt_entry = &pdpt->entries[PDPT_INDEX(virtual_address)];

    if (*pdpt_entry & PAGE_PRESENT)
    {
        if (*pdpt_entry & PAGE_HUGE)
        {
            if (create)
            {
                if (status_out)
                {
                    *status_out = PAGE_WALK_HUGE_PAGE_PDPT;
                }

                return NULL;
            }

            if (level_out)
            {
                *level_out = PAGE_WALK_LEVEL_PDPT;
            }

            return pdpt_entry;
        }
    }
    else if (!create)
    {
        if (status_out)
        {
            *status_out = PAGE_WALK_MISSING_PDPT_ENTRY;
        }

        return NULL;
    }

    page_table_t *pd = ensure_next_table(
        pdpt,
        PDPT_INDEX(virtual_address),
        create,
        PAGE_WALK_MISSING_PDPT_ENTRY,
        status_out
    );

    if (pd == NULL)
    {
        return NULL;
    }

    page_entry_t *pd_entry = &pd->entries[PD_INDEX(virtual_address)];

    if (*pd_entry & PAGE_PRESENT)
    {
        if (*pd_entry & PAGE_HUGE)
        {
            if (create)
            {
                if (status_out)
                {
                    *status_out = PAGE_WALK_HUGE_PAGE_PD;
                }

                return NULL;
            }

            if (level_out)
            {
                *level_out = PAGE_WALK_LEVEL_PD;
            }

            return pd_entry;
        }
    }
    else if (!create)
    {
        if (status_out)
        {
            *status_out = PAGE_WALK_MISSING_PD_ENTRY;
        }

        return NULL;
    }

    page_table_t *pt = ensure_next_table(
        pd,
        PD_INDEX(virtual_address),
        create,
        PAGE_WALK_MISSING_PD_ENTRY,
        status_out
    );

    if (pt == NULL)
    {
        return NULL;
    }

    page_entry_t *pt_entry = &pt->entries[PT_INDEX(virtual_address)];

    if (!create && !(*pt_entry & PAGE_PRESENT))
    {
        if (status_out)
        {
            *status_out = PAGE_WALK_MISSING_PT_ENTRY;
        }

        return NULL;
    }

    if (level_out)
    {
        *level_out = PAGE_WALK_LEVEL_PT;
    }

    return pt_entry;
}

void page_table_set_hhdm_offset(uint64_t offset)
{
    hhdm_offset = offset;
    hhdm_ready = true;
}

void *page_table_physical_to_virtual(uint64_t physical_address)
{
    return (void *)(uintptr_t)(physical_address + hhdm_offset);
}

page_table_t *page_table_get_pml4(void)
{
    return page_table_active_root();
}

page_table_t *page_table_create(uint64_t *physical_out)
{
    void *physical_page = pmm_alloc_page();
    kprintf("[PT] allocated PA = %p\n", physical_page);

    if (physical_page == NULL)
    {
        return NULL;
    }

    if (physical_out !=NULL)
    {
        *physical_out = (uint64_t)(uintptr_t)physical_page;
        kprintf("[PT] stored PA = %p\n", (void *)*physical_out);
    }

    page_table_t *new_pml4 = (page_table_t *)page_table_physical_to_virtual((uint64_t)(uintptr_t)physical_page);

    if (new_pml4 == NULL)
    {
        pmm_free_page(physical_page);
        return NULL;
    }

    k_memset(new_pml4, 0, PAGE_SIZE);

    if (kernel_pml4 != NULL)
    {
        k_memcpy(new_pml4, kernel_pml4, PAGE_SIZE);
    }

    return new_pml4;
}

void page_table_init(void)
{
    if (kernel_pml4 != NULL)
    {
        return;
    }

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

    if (kernel_pml4 == NULL)
    {
        kprintf("Failed to convert PML4 to virtual address\n");
        pmm_free_page(pml4_physical);
        kernel_pml4_physical = 0;
        return;
    }

    kprintf("Kernel PML4 PA : %p\n", (void *)kernel_pml4_physical);

    page_table_t *current_pml4 = page_table_current_root();

    if (current_pml4 == NULL)
    {
        kprintf("Failed to read active page tables\n");
        pmm_free_page(pml4_physical);
        kernel_pml4 = NULL;
        kernel_pml4_physical = 0;
        return;
    }

    uint64_t current_cr3 = read_cr3() & PAGE_TABLE_ADDRESS_MASK;
    kprintf("Current CR3      : %p\n", (void *)current_cr3);
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
    page_walk_level_t level = PAGE_WALK_LEVEL_NONE;
    page_walk_status_t status = PAGE_WALK_OK;
    page_entry_t *entry = page_walk_internal(
        page_table_active_root(),
        virtual_address,
        true,
        &level,
        &status
    );

    if (entry == NULL)
    {
        kprintf("page_table_map failed: %s\n", page_walk_status_name(status));
        return false;
    }

    if (level != PAGE_WALK_LEVEL_PT)
    {
        kprintf("page_table_map refused to replace %s mapping\n", page_walk_level_name(level));
        return false;
    }

    *entry = (physical_address & PAGE_TABLE_ADDRESS_MASK) |
        (flags & ~PAGE_PRESENT & ~PAGE_HUGE) |
        PAGE_PRESENT;

    invalidate_page(virtual_address);

    return true;

}

bool page_table_map_user(
    page_table_t *pml4,
    uint64_t virtual_address,
    uint64_t physical_address,
    bool writable
)
{
    uint64_t flags = PAGE_PRESENT | PAGE_USER;

    if (writable)
    {
        flags |= PAGE_WRITABLE;
    }

    page_entry_t *entry = page_walk(pml4, virtual_address, true);

    if (entry == NULL)
    {
        return false;
    }

    page_table_t *pdpt = (page_table_t *)page_table_physical_to_virtual(
        pml4->entries[PML4_INDEX(virtual_address)]&
        PAGE_TABLE_ADDRESS_MASK
    );

    pml4->entries[PML4_INDEX(virtual_address)] |= PAGE_USER;

    page_table_t *pd = (page_table_t *)page_table_physical_to_virtual(
        pdpt->entries[PDPT_INDEX(virtual_address)]&
        PAGE_TABLE_ADDRESS_MASK
    );

    pdpt->entries[PDPT_INDEX(virtual_address)] |= PAGE_USER;

    page_table_t *pt = (page_table_t *)page_table_physical_to_virtual(
        pd->entries[PD_INDEX(virtual_address)]&
        PAGE_TABLE_ADDRESS_MASK
    );

    pd->entries[PD_INDEX(virtual_address)] |= PAGE_USER;

    *entry = physical_address | flags;

    return true;
}

bool page_table_map_page(uint64_t virtual_address, uint64_t physical_address, bool writable)
{
    uint64_t flags = 0;

    if (writable)
    {
        flags |= PAGE_WRITABLE;
    }

    return page_table_map(
        virtual_address,
        physical_address,
        flags
    );
}

bool page_table_unmap(uint64_t virtual_address)
{
    page_walk_level_t level = PAGE_WALK_LEVEL_NONE;
    page_walk_status_t status = PAGE_WALK_OK;
    page_entry_t *entry = page_walk_internal(
        page_table_active_root(),
        virtual_address,
        false,
        &level,
        &status
    );

    if (entry == NULL)
    {
        kprintf("page_table_unmap failed: %s\n", page_walk_status_name(status));
        return false;
    }

    if (level != PAGE_WALK_LEVEL_PT)
    {
        kprintf("page_table_unmap refused to clear %s mapping\n", page_walk_level_name(level));
        return false;
    }

    if (!(*entry & PAGE_PRESENT))
    {
        return false;
    }

    *entry = 0;

    invalidate_page(virtual_address);

    return true;

}

uint64_t page_table_translate(uint64_t virtual_address)
{
    page_walk_level_t level = PAGE_WALK_LEVEL_NONE;
    page_walk_status_t status = PAGE_WALK_OK;
    page_entry_t *entry = page_walk_internal(
        page_table_active_root(),
        virtual_address,
        false,
        &level,
        &status
    );

    if (entry == NULL)
    {
        return 0;
    }

    if (!(*entry & PAGE_PRESENT))
    {
        return 0;
    }

    uint64_t page_size = page_size_for_level(level);

    if (page_size == 0)
    {
        return 0;
    }

    uint64_t physical = *entry & PAGE_TABLE_ADDRESS_MASK;

    return physical | (virtual_address & (page_size - 1ULL));
}

void page_table_activate(void)
{
    if (kernel_pml4_physical == 0)
    {
        kprintf("Cannot activate page tables before initialization\n");
        return;
    }

    kprintf("Switching to ZenithOS page tables...\n");

    write_cr3(kernel_pml4_physical);

    active_pml4_physical = kernel_pml4_physical;

    kprintf("CR3 switched successfully!\n");
}

void page_table_switch(uint64_t physical_pml4)
{
    if (physical_pml4 == 0)
    {
        kprintf("[PAGING] Invalid CR3 switch request\n");
        return;
    }

    if (active_pml4_physical == physical_pml4)
    {
        return;
    }

    write_cr3(physical_pml4);

    active_pml4_physical = physical_pml4;
}

page_entry_t *page_walk(
    page_table_t *pml4,
    uint64_t virtual_address,
    bool create
)
{
    return page_walk_internal(
        pml4,
        virtual_address,
        create,
        NULL,
        NULL
    );
}

bool page_table_is_mapped(uint64_t virtual_address)
{
    kprintf("\nChecking VA : %p\n", (void *)virtual_address);

    page_walk_level_t level = PAGE_WALK_LEVEL_NONE;
    page_walk_status_t status = PAGE_WALK_OK;
    page_entry_t *entry = page_walk_internal(
        page_table_get_pml4(),
        virtual_address,
        false,
        &level,
        &status
    );

    if (entry == NULL)
    {
        kprintf("page_walk -> NULL (%s)\n", page_walk_status_name(status));
        return false;
    }

    kprintf("Entry Ptr   : %p\n", entry);
    kprintf("Entry Value : %016llx\n", *entry);
    kprintf("Leaf Level  : %s\n", page_walk_level_name(level));

    if (*entry & PAGE_PRESENT)
    {
        kprintf("PAGE_PRESENT = YES\n");
    }
    else
    {
        kprintf("PAGE_PRESENT = NO\n");
    }

    return (*entry & PAGE_PRESENT) != 0;
}

void page_table_dump(uint64_t virtual_address)
{
    page_walk_level_t level = PAGE_WALK_LEVEL_NONE;
    page_walk_status_t status = PAGE_WALK_OK;
    page_entry_t *entry = page_walk_internal(
        page_table_get_pml4(),
        virtual_address,
        false,
        &level,
        &status
    );

    kprintf("\n===== Page Table Dump =====\n");
    kprintf("Virtual Address : %p\n", (void *)virtual_address);
    kprintf("PML4 Index      : %u\n", (unsigned)PML4_INDEX(virtual_address));
    kprintf("PDPT Index      : %u\n", (unsigned)PDPT_INDEX(virtual_address));
    kprintf("PD Index        : %u\n", (unsigned)PD_INDEX(virtual_address));
    kprintf("PT Index        : %u\n", (unsigned)PT_INDEX(virtual_address));

    if (entry == NULL)
    {
        kprintf("Walk Status     : %s\n", page_walk_status_name(status));
        kprintf("============================\n");
        return;
    }

    kprintf("Leaf Level      : %s\n", page_walk_level_name(level));
    kprintf("Entry Pointer   : %p\n", entry);
    kprintf("Entry Value     : %016llx\n", *entry);
    kprintf("Present         : %s\n", page_is_present(*entry) ? "YES" : "NO");
    kprintf("Writable        : %s\n", page_is_writable(*entry) ? "YES" : "NO");
    kprintf("User            : %s\n", page_is_user(*entry) ? "YES" : "NO");
    kprintf("Executable      : %s\n", page_is_executable(*entry) ? "YES" : "NO");
    kprintf("============================\n");
}
