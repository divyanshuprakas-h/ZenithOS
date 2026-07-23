#include "demand_paging.h"

#include "../mm/pmm.h"
#include "page_table.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"

#include "vma/vma.h"

bool vmm_handle_page_fault(
    uint64_t fault_address,
    uint64_t error_code
)
{
    bool present = error_code & (1 << 0);

    kprintf("[DP] Page fault received\n");

    if (present)
    {
        kprintf("[DP] Protection fault - cannot recover\n");
        return false;
    }

    const vma_t *vma = vma_find(fault_address);

    if (vma == NULL)
    {
        kprintf("[DP] No VMA found \n");
        return false;
    }

    if (!(vma->flags & VMA_DEMAND_PAGED))
    {
        kprintf("[DP] VMA is not demand paged \n");
        return false;
    }

    uint64_t page_base = fault_address & ~(PAGE_SIZE - 1);

    kprintf("[DP] Fault Address : %p\n", (void *)fault_address);
    kprintf("[DP] Page Base     : %p\n", (void *)page_base);

    void *physical_page = pmm_alloc_page();

    if (physical_page == NULL)
    {
        kprintf("[DP] PMM allocation failed\n");
        return false;
    }

    kprintf("[DP] Allocated physical page: %p\n", physical_page);

    void *virtual_page =
        page_table_physical_to_virtual(
            (uint64_t)(uintptr_t)physical_page
        );

    k_memset(virtual_page, 0, PAGE_SIZE);

    kprintf("[DP] Page cleared\n");

    if (!page_table_map(
            page_base,
            (uint64_t)(uintptr_t)physical_page,
            PAGE_WRITABLE))
    {
        kprintf("[DP] page_table_map() failed\n");

        pmm_free_page(physical_page);
        return false;
    }

    kprintf("[DP] Mapping successful\n");

    kprintf("[DP] Recovery complete\n");

    return true;
}