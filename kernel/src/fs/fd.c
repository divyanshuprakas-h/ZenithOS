#include "fd.h"

#include "../lib/memory.h"
#include "../kernel_err/errno.h"

#include <stddef.h>

void fd_init(fd_table_t *table)
{
    if (table == NULL)
        return;

    k_memset(table, 0, sizeof(fd_table_t));
}

file_t *fd_get(fd_table_t *table, int fd)
{
    if (table == NULL)
        return NULL;

    if (fd < 0 || fd >= FD_MAX)
        return NULL;

    return table->files[fd];
}

int fd_allocate(fd_table_t *table, file_t *file)
{
    if (table == NULL || file == NULL)
        return KERNEL_EINVAL;

    for (int fd = 0; fd < FD_MAX; fd++)
    {
        if (table->files[fd] == NULL)
        {
            table->files[fd] = file;
            return fd;
        }
    }
    return KERNEL_ENOMEM;
}

int fd_close(fd_table_t *table, int fd)
{
    if (table == NULL)
        return KERNEL_EINVAL;

    if (fd < 0 || fd >= FD_MAX)
        return KERNEL_EINVAL;

    if (table->files[fd] == NULL)
        return KERNEL_EINVAL;
        
    table->files[fd] = NULL;

    return KERNEL_SUCCESS;
}