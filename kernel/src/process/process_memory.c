#include "process.h"

#include "../mm/pmm.h"
#include "../vmm/page_table.h"
#include "../vmm/user_vm_layout.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"

#include <stddef.h>

int process_map_initial_stack(process_t *process)
{
    if (process == NULL)
        return KERNEL_EINVAL;

    void *page = pmm_alloc_page();

    if (page == NULL)
        return KERNEL_ENOMEM;

    uint64_t stack_page = process->user_stack_top - PAGE_SIZE;

    if (!page_table_map_user(
        process->page_table,
        stack_page,
        (uint64_t)(uintptr_t)page,
        true
    ))
    {
        pmm_free_page(page);
        return KERNEL_ENOMEM;
    }

    kprintf("[STACK] mapped VA=0x%llx PA=%p\n", (unsigned long long)stack_page, page);
    return KERNEL_SUCCESS;
}