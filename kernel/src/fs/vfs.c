#include "vfs.h"
#include "mount.h"
#include "filesystem.h"
#include "vnode.h"
#include "path.h"

#include "kernel_err/errno.h"
#include "../mm/heap.h"

#include <stddef.h>

static mount_t *vfs_mount_list = NULL;
static mount_t *vfs_root_mount = NULL;

void vfs_init(void)
{
    vfs_mount_list = NULL;
    vfs_root_mount = NULL;
}

int vfs_mount(filesystem_t *fs, vnode_t *mountpoint)
{
    if (fs == NULL)
    {
        return KERNEL_EINVAL;
    }

    if (vfs_root_mount != NULL && mountpoint == NULL)
    {
        return KERNEL_EINVAL;
    }


    mount_t *mount = kmalloc(sizeof(mount_t));

    if (mount == NULL)
    {
        return KERNEL_ENOMEM;
    }

    mount->fs = fs;
    mount->mountpoint = mountpoint;

    mount->next = vfs_mount_list;
    vfs_mount_list = mount;

    if (vfs_root_mount == NULL)
    {
        mount->mountpoint = NULL;
        vfs_root_mount = mount;
    }

    if (fs->ops != NULL && fs->ops->mount != NULL)
    {
        int result = fs->ops->mount(fs);

        if (result != KERNEL_SUCCESS)
        {
            return result;
        }
    }

    return KERNEL_SUCCESS;

}

vnode_t *vfs_get_root(void)
{
    if (vfs_root_mount == NULL)
    {
        return NULL;
    }

    if (vfs_root_mount->fs == NULL)
    {
        return NULL;
    }

    return vfs_root_mount->fs->root;

}

int vfs_lookup(const char *path, vnode_t **result)
{
    if (path == NULL || result == NULL)
    {
        return KERNEL_EINVAL;
    }

    vnode_t *current = vfs_get_root();

    if (current == NULL)
    {
        return KERNEL_ENOENT;
    }

    if (path[0] == '/' && path[1] == '\0')
    {
        *result = current;
        return KERNEL_SUCCESS;
    }

    char component[VFS_MAX_COMPONENT_LEN + 1];

    const char *cursor = path;

    vnode_t *next;

    while (1)
    {
        int rc = path_next_component(&cursor, component);

        if (rc == 0)
            break;
        
        if (rc < 0)
            return KERNEL_EINVAL;

        if (current->ops == NULL || current->ops->lookup == NULL)
            return KERNEL_ENOTSUP;

        rc = current->ops->lookup(current, component, &next);

        if (rc != KERNEL_SUCCESS)
            return rc;

        current = next;
    }

    *result = current;

    return KERNEL_SUCCESS;

}

int vfs_open(const char *path, file_t **result)
{
    if (path == NULL || result == NULL)
    {
        return KERNEL_EINVAL;
    }

    vnode_t *node;

    int rc = vfs_lookup(path, &node);

    if (rc != KERNEL_SUCCESS)
        return rc;

    return file_open(node, result);

}

int vfs_write(file_t *file, const void *buffer, uint64_t size)
{
    if (file == NULL)
        return KERNEL_EINVAL;

    if (file->node == NULL)
        return KERNEL_EINVAL;

    if (file->node->ops == NULL)
        return KERNEL_ENOTSUP;

    if (file->node->ops->write == NULL)
        return KERNEL_ENOTSUP;

    return file->node->ops->write(file, buffer, size);
}

int vfs_read(file_t *file, void *buffer, uint64_t size)
{
    if (file == NULL)
        return KERNEL_EINVAL;

    if (file->node == NULL)
        return KERNEL_EINVAL;

    if (file->node->ops == NULL)
        return KERNEL_ENOTSUP;

    if (file->node->ops->read == NULL)
        return KERNEL_ENOTSUP;

    return file->node->ops->read(file, buffer, size);
}