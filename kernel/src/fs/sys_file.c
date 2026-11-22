#include "sys_file.h"

#include "vfs.h"
#include "fd.h"

#include "../process/process.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../terminal/terminal.h"
#include "../hal/io.h"

#include "../scheduler/scheduler.h"
#include "../gui/gui_state.h"
#include <stddef.h>

int sys_open(const char *path)
{
    if (path == NULL)
        return KERNEL_EINVAL;
    
    process_t *process = process_current();

    if (process == NULL)
        return KERNEL_EINVAL;

    file_t *file;

    int status = vfs_open(path, &file);

    if (status != KERNEL_SUCCESS)
        return status;

    return fd_allocate(&process->fd_table, file); 

}

int sys_read(int fd, void *buffer, uint64_t size)
{
    process_t *process = process_current();

    if (process == NULL)
        return KERNEL_EINVAL;

    if (buffer == NULL && size > 0)
        return KERNEL_EINVAL;

    if (fd == 0)
    {
        if (size == 0)
            return 0;

        while (!terminal_stdin_has_data())
        {
            __asm__ volatile("sti");
            scheduler_yield();
        }

        return terminal_stdin_read((char *)buffer, (size_t)size);
    }

    file_t *file = fd_get(&process->fd_table, fd);

    if (file == NULL)
        return KERNEL_ENOENT;

    return vfs_read(file, buffer, size);
}

int sys_write(int fd, void *buffer, uint64_t size)
{
    process_t *process = process_current();

    if (process == NULL)
        return KERNEL_EINVAL;

    if (buffer == NULL && size > 0)
        return KERNEL_EINVAL;

    if (fd == 1 || fd == 2)
    {
        const char *buf = (const char *)buffer;
        for (uint64_t i = 0; i < size; i++)
        {
            terminal_putchar(buf[i]);
            outb(0xE9, (uint8_t)buf[i]);
        }
        terminal_render();
        return (int)size;
    }

    file_t *file = fd_get(&process->fd_table, fd);

    if (file == NULL)
        return KERNEL_ENOENT;

    return vfs_write(file, buffer, size);
}

int sys_close(int fd)
{
    if (fd == 0 || fd == 1 || fd == 2)
    {
        return 0;
    }

    process_t *process = process_current();

    if (process == NULL)
    {
        return KERNEL_EINVAL;
    }

    return fd_close(&process->fd_table, fd);
}

int sys_readdir(const char *path, void *buffer, uint64_t max_entries)
{
    if (path == NULL || buffer == NULL || max_entries == 0)
    {
        return -KERNEL_EINVAL;
    }

    size_t out_count = 0;
    int rc = vfs_read_dir(path, (vfs_dirent_t *)buffer, (size_t)max_entries, &out_count);
    if (rc != KERNEL_SUCCESS)
    {
        return -rc;
    }

    return (int)out_count;
}

int sys_create(const char *path)
{
    if (path == NULL)
        return -KERNEL_EINVAL;
    int rc = vfs_create(path);
    return rc == KERNEL_SUCCESS ? 0 : -rc;
}

int sys_mkdir(const char *path)
{
    if (path == NULL)
        return -KERNEL_EINVAL;
    int rc = vfs_mkdir(path);
    return rc == KERNEL_SUCCESS ? 0 : -rc;
}

int sys_unlink(const char *path)
{
    if (path == NULL)
        return -KERNEL_EINVAL;
    int rc = vfs_unlink(path);
    return rc == KERNEL_SUCCESS ? 0 : -rc;
}
