#include "file.h"

#include "../kernel_err/errno.h"
#include "../mm/heap.h"
#include "../lib/memory.h"

int file_open(vnode_t *node, file_t **result)
{
    if (node == NULL || result == NULL)
    {
        return KERNEL_EINVAL;
    }

    file_t *file = kmalloc(sizeof(file_t));

    if (file == NULL)
        return KERNEL_ENOMEM;

    k_memset(file, 0, sizeof(file_t));

    file->node = node;

    file->position = 0;

    file->flags = 0;

    file->refcount = 1;

    *result = file;

    return KERNEL_SUCCESS;
}

void file_close(file_t *file)
{
    if (file == NULL)
        return;

    kfree(file);
}