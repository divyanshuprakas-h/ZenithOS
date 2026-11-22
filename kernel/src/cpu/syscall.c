#include "syscall.h"

#include "../fs/sys_file.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../mm/pmm.h"
#include "../gui/gui_state.h"
#include "user_access.h"

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
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != 0)
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_open(path_buf);
            break;
        }

        case SYS_READ:
            if (!user_ptr_valid((void *)arg2, arg3))
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_read((int)arg1, (void *)arg2, arg3);
            break;

        case SYS_WRITE:
            if (!user_ptr_valid((const void *)arg2, arg3))
            {
                ret = -KERNEL_EINVAL;
                break;
            }
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
            if (!user_ptr_valid((void *)arg1, sizeof(sysinfo_t)))
            {
                ret = -KERNEL_EINVAL;
            }
            else
            {
                sysinfo_t info;
                info.total_pages = pmm_get_total_pages();
                info.free_pages = pmm_get_free_pages();
                info.used_pages = pmm_get_used_pages();
                info.page_size = 4096;
                copy_to_user((void *)arg1, &info, sizeof(sysinfo_t));
                ret = 0;
            }
            break;

        case SYS_READDIR:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != 0 ||
                !user_ptr_valid((void *)arg2, arg3 * 72))
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_readdir(path_buf, (void *)arg2, arg3);
            break;
        }

        case SYS_CREATE:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != 0)
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_create(path_buf);
            break;
        }

        case SYS_MKDIR:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != 0)
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_mkdir(path_buf);
            break;
        }

        case SYS_UNLINK:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != 0)
            {
                ret = -KERNEL_EINVAL;
                break;
            }
            ret = sys_unlink(path_buf);
            break;
        }

        case SYS_SET_THEME:
            if (arg1 == (uint64_t)-1)
            {
                ret = gui_get_theme();
            }
            else
            {
                gui_set_theme((int)arg1);
                ret = 0;
            }
            break;

        default:
            kprintf("[SYSCALL] Unknown syscall: %llu\n", (unsigned long long)syscall_num);
            ret = -KERNEL_EINVAL;
            break;
    }

    context->regs.rax = (uint64_t)ret;
}
