#include "syscall.h"

#include "../fs/sys_file.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../mm/pmm.h"

void syscall_dispatch(interrupt_context_t *context)
{
    if (context == NULL)
    {
        return;
    }

    uint64_t syscall_num = context->regs.rdi;
    uint64_t arg1 = context->regs.rsi;
    uint64_t arg2 = context->regs.rdx;
    uint64_t arg3 = context->regs.rcx;

    int64_t ret = -KERNEL_EINVAL;

    switch (syscall_num)
    {
        case SYS_EXIT:
            process_exit((int)arg1);
            ret = 0;
            break;

        case SYS_OPEN:
            ret = sys_open((const char *)arg1);
            break;

        case SYS_READ:
            ret = sys_read((int)arg1, (void *)arg2, arg3);
            break;

        case SYS_WRITE:
            ret = sys_write((int)arg1, (void *)arg2, arg3);
            break;

        case SYS_CLOSE:
            ret = sys_close((int)arg1);
            break;

        case SYS_YIELD:
            scheduler_yield();
            ret = 0;
            break;

        case SYS_SYSINFO:
            if (arg1 == 0)
            {
                ret = -KERNEL_EINVAL;
            }
            else
            {
                sysinfo_t *info = (sysinfo_t *)arg1;
                info->total_pages = pmm_get_total_pages();
                info->free_pages = pmm_get_free_pages();
                info->used_pages = pmm_get_used_pages();
                info->page_size = 4096;
                ret = 0;
            }
            break;

        case SYS_READDIR:
            ret = sys_readdir((const char *)arg1, (void *)arg2, arg3);
            break;

        default:
            kprintf("[SYSCALL] Unknown syscall: %llu\n", (unsigned long long)syscall_num);
            ret = -KERNEL_EINVAL;
            break;
    }

    context->regs.rax = (uint64_t)ret;
}
