#include "sys_file.h"

#include "vfs.h"
#include "fd.h"

#include "../process/process.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"

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