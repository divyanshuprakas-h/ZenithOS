#include "page_table.h"

#include "../mm/pmm.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"

static page_table_t *kernel_pml4 = NULL;
static uint64_t hhdm_offset;

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

    kernel_pml4 = (page_table_t *)page_table_physical_to_virtual(
        (uint64_t)(uintptr_t)pml4_physical
    );

    k_memset(kernel_pml4, 0, PAGE_SIZE);

    kprintf("Kernel PML4  : %p\n", kernel_pml4);

}
