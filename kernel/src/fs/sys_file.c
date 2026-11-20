#include "sys_file.h"

#include "vfs.h"
#include "fd.h"

#include "../process/process.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../terminal/terminal.h"
#include "../hal/io.h"

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
    process_t *process = process_current();

    if (process == NULL)
    {
        return KERNEL_EINVAL;
    }

    return fd_close(&process->fd_table, fd);
}