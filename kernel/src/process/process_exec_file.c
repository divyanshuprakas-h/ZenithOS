#include "process_exec_file.h"
#include "process_exec.h"

#include "../fs/vfs.h"
#include "../fs/file.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../mm/heap.h"
#include "../lib/memory.h"

#include <stddef.h>

int process_exec_file(const char *path, process_t **process)
{
    if (process == NULL || path == NULL)
        return KERNEL_EINVAL;

    file_t *file = NULL;
    int open_status = vfs_open(path, &file);
    if (open_status != KERNEL_SUCCESS)
    {
        return open_status;
    }

    size_t file_size = (file->node != NULL) ? file->node->size : 0;
    if (file_size < sizeof(elf64_header_t) || file_size > 16 * 1024 * 1024)
    {
        vfs_close(file);
        return KERNEL_EINVAL;
    }

    void *buffer = kmalloc(file_size);
    if (buffer == NULL)
    {
        vfs_close(file);
        return KERNEL_ENOMEM;
    }

    int bytes = vfs_read(file, buffer, file_size);
    vfs_close(file);

    if (bytes < 0 || (size_t)bytes < file_size)
    {
        kfree(buffer);
        return bytes < 0 ? bytes : KERNEL_EIO;
    }

    *process = process_create_empty("init");
    if (*process == NULL)
    {
        kfree(buffer);
        return KERNEL_ENOMEM;
    }

    int status = process_exec(*process, buffer, file_size);
    kfree(buffer);

    if (status != KERNEL_SUCCESS)
    {
        process_destroy(*process);
        *process = NULL;
        return status;
    }

    return KERNEL_SUCCESS;
}
