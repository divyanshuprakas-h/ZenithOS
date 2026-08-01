#include "process_heap.h"

#include "../mm/pmm.h"
#include "../vmm/page_table.h"
#include "../vmm/user_vm_layout.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"

#include <stddef.h>
#include <stdint.h>

int process_heap_expand(process_t *process)
{
    if (process == NULL)
        return KERNEL_EINVAL;

    void *page = pmm_alloc_page();

    if (page == NULL)
        return KERNEL_ENOMEM;

    uint64_t heap_page = process->user_heap_end;

    if (!page_table_map_user(
        process->page_table,
        heap_page,
        (uint64_t)(uintptr_t)page,
        true
    ))
    {
        pmm_free_page(page);
        return KERNEL_ENOMEM;
    }

    process->user_heap_end += PAGE_SIZE;
    process->user_heap_current = process->user_heap_end;

    kprintf("[HEAP] mapped VA=0x%llx PA=%p\n", (unsigned long long)heap_page, page);
    
    return KERNEL_SUCCESS;
}