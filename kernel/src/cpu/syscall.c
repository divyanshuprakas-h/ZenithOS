#include "syscall.h"

#include "../fs/sys_file.h"
#include "../fs/vfs.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../gui/gui_state.h"
#include "user_access.h"
#include "../drivers/serial.h"

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

    int64_t ret = KERNEL_EINVAL;

    switch (syscall_num)
    {
        case SYS_EXIT:
            process_exit((int)arg1);
            ret = KERNEL_SUCCESS;
            break;

        case SYS_OPEN:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != KERNEL_SUCCESS)
            {
                ret = KERNEL_EINVAL;
                break;
            }
            ret = sys_open(path_buf);
            break;
        }

        case SYS_READ:
        {
            if (arg3 == 0)
            {
                ret = 0;
                break;
            }
            if (!user_ptr_accessible((void *)arg2, arg3, true))
            {
                ret = KERNEL_EINVAL;
                break;
            }

            // Read via bounded kernel buffer chunks so drivers/VFS never touch user pointers directly
            uint8_t kbuf[512];
            uint64_t remaining = arg3;
            uintptr_t uptr = arg2;
            int64_t total_read = 0;

            while (remaining > 0)
            {
                uint64_t to_read = remaining < sizeof(kbuf) ? remaining : sizeof(kbuf);
                int bytes = sys_read((int)arg1, kbuf, to_read);
                if (bytes < 0)
                {
                    if (total_read == 0)
                    {
                        total_read = bytes;
                    }
                    break;
                }
                if (bytes == 0)
                {
                    break;
                }
                if (copy_to_user((void *)uptr, kbuf, (size_t)bytes) != KERNEL_SUCCESS)
                {
                    if (total_read == 0)
                    {
                        total_read = KERNEL_EINVAL;
                    }
                    break;
                }
                total_read += bytes;
                uptr += bytes;
                remaining -= bytes;
                if ((size_t)bytes < to_read)
                {
                    // Partial read (line or EOF or packet), do not block
                    break;
                }
            }
            ret = total_read;
            break;
        }

        case SYS_WRITE:
        {
            if (arg3 == 0)
            {
                ret = 0;
                break;
            }
            if (!user_ptr_accessible((const void *)arg2, arg3, false))
            {
                ret = KERNEL_EINVAL;
                break;
            }

            // Write via bounded kernel buffer chunks so drivers/VFS never touch user pointers directly
            uint8_t kbuf[512];
            uint64_t remaining = arg3;
            uintptr_t uptr = arg2;
            int64_t total_written = 0;

            while (remaining > 0)
            {
                uint64_t to_write = remaining < sizeof(kbuf) ? remaining : sizeof(kbuf);
                if (copy_from_user(kbuf, (const void *)uptr, (size_t)to_write) != KERNEL_SUCCESS)
                {
                    if (total_written == 0)
                    {
                        total_written = KERNEL_EINVAL;
                    }
                    break;
                }
                int bytes = sys_write((int)arg1, kbuf, to_write);
                if (bytes < 0)
                {
                    if (total_written == 0)
                    {
                        total_written = bytes;
                    }
                    break;
                }
                total_written += bytes;
                uptr += bytes;
                remaining -= bytes;
                if ((size_t)bytes < to_write)
                {
                    break;
                }
            }
            ret = total_written;
            break;
        }

        case SYS_CLOSE:
            ret = sys_close((int)arg1);
            break;

        case SYS_YIELD:
            scheduler_yield();
            ret = KERNEL_SUCCESS;
            break;

        case SYS_SYSINFO:
        {
            if (!user_ptr_accessible((void *)arg1, sizeof(sysinfo_t), true))
            {
                ret = KERNEL_EINVAL;
            }
            else
            {
                sysinfo_t info;
                info.total_pages = pmm_get_total_pages();
                info.free_pages = pmm_get_free_pages();
                info.used_pages = pmm_get_used_pages();
                info.page_size = 4096;
                if (copy_to_user((void *)arg1, &info, sizeof(sysinfo_t)) != KERNEL_SUCCESS)
                {
                    ret = KERNEL_EINVAL;
                }
                else
                {
                    ret = KERNEL_SUCCESS;
                }
            }
            break;
        }

        case SYS_READDIR:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != KERNEL_SUCCESS)
            {
                ret = KERNEL_EINVAL;
                break;
            }

            if (arg3 == 0 || arg3 > 512)
            {
                ret = KERNEL_EINVAL;
                break;
            }

            // Check multiplication overflow for user buffer size
            uint64_t user_buf_size = arg3 * sizeof(vfs_dirent_t);
            if (user_buf_size / sizeof(vfs_dirent_t) != arg3)
            {
                ret = KERNEL_EINVAL;
                break;
            }

            if (!user_ptr_accessible((void *)arg2, user_buf_size, true))
            {
                ret = KERNEL_EINVAL;
                break;
            }

            if (arg3 <= 32)
            {
                vfs_dirent_t k_entries[32];
                int count = sys_readdir(path_buf, k_entries, arg3);
                if (count < 0)
                {
                    ret = count;
                }
                else
                {
                    if (count > 0)
                    {
                        if (copy_to_user((void *)arg2, k_entries, (size_t)count * sizeof(vfs_dirent_t)) != KERNEL_SUCCESS)
                        {
                            ret = KERNEL_EINVAL;
                            break;
                        }
                    }
                    ret = count;
                }
            }
            else
            {
                vfs_dirent_t *dynamic_buf = kmalloc((size_t)user_buf_size);
                if (dynamic_buf == NULL)
                {
                    ret = KERNEL_ENOMEM;
                    break;
                }
                int count = sys_readdir(path_buf, dynamic_buf, arg3);
                if (count > 0)
                {
                    if (copy_to_user((void *)arg2, dynamic_buf, (size_t)count * sizeof(vfs_dirent_t)) != KERNEL_SUCCESS)
                    {
                        count = KERNEL_EINVAL;
                    }
                }
                kfree(dynamic_buf);
                ret = count;
            }
            break;
        }

        case SYS_CREATE:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != KERNEL_SUCCESS)
            {
                ret = KERNEL_EINVAL;
                break;
            }
            ret = sys_create(path_buf);
            break;
        }

        case SYS_MKDIR:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != KERNEL_SUCCESS)
            {
                ret = KERNEL_EINVAL;
                break;
            }
            ret = sys_mkdir(path_buf);
            break;
        }

        case SYS_UNLINK:
        {
            char path_buf[128];
            if (copy_str_from_user(path_buf, (const char *)arg1, sizeof(path_buf)) != KERNEL_SUCCESS)
            {
                ret = KERNEL_EINVAL;
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
                ret = KERNEL_SUCCESS;
            }
            break;

        case SYS_SERIAL_STATUS:
            if (!user_ptr_accessible((void *)arg1, sizeof(serial_status_t), true))
            {
                ret = KERNEL_EINVAL;
            }
            else
            {
                serial_status_t status;
                serial_get_status(&status);
                if (copy_to_user((void *)arg1, &status, sizeof(serial_status_t)) != KERNEL_SUCCESS)
                {
                    ret = KERNEL_EINVAL;
                }
                else
                {
                    ret = KERNEL_SUCCESS;
                }
            }
            break;

        default:
            kprintf("[SYSCALL] Unknown syscall: %llu\n", (unsigned long long)syscall_num);
            ret = KERNEL_ENOTSUP;
            break;
    }

    context->regs.rax = (uint64_t)ret;
}
