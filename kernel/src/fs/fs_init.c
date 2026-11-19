#include "vfs.h"
#include "ramfs.h"

#include "../stdio/printf.h"
#include "../kernel_err/errno.h"

#include <stddef.h>

static filesystem_t root_fs;

int fs_init(void)
{
    vfs_init();

    if (ramfs_init(&root_fs) != KERNEL_SUCCESS)
    {
        return KERNEL_EINVAL;
    }

    if (vfs_mount(&root_fs, NULL) != KERNEL_SUCCESS)
    {
        return KERNEL_EINVAL;
    }

    kprintf("[FS] Root filesystem mounted\n");
    return KERNEL_SUCCESS;
}