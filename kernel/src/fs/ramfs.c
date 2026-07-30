#include "ramfs.h"
#include "vnode.h"
#include "filesystem.h"
#include "file.h"

#include "../kernel_err/errno.h"
#include "../lib/string.h"
#include "../mm/heap.h"
#include "../lib/memory.h"

#include <stddef.h>

static ramfs_node_t ramfs_root;

static vnode_operations_t ramfs_vnode_ops = 
{
    .lookup = ramfs_lookup,
    .create = ramfs_create,
    .mkdir = ramfs_mkdir,
    .read = ramfs_read,
    .write = ramfs_write,
    .remove = NULL
};

static filesystem_operations_t ramfs_fs_ops =
{
    .mount = ramfs_mount,
    .unmount = NULL,
    .sync = NULL
};

int ramfs_mount(filesystem_t *fs)
{
    if (fs == NULL)
    {
        return KERNEL_EINVAL;
    }

    k_memset(&ramfs_root, 0, sizeof(ramfs_root));

    ramfs_root.data = NULL;

    ramfs_root.capacity = 0;

    ramfs_root.vnode.type = VNODE_DIRECTORY;

    ramfs_root.vnode.size = 0;

    ramfs_root.vnode.parent = NULL;

    ramfs_root.vnode.fs = fs;

    ramfs_root.vnode.ops = &ramfs_vnode_ops;

    ramfs_root.vnode.private_data = &ramfs_root;

    ramfs_root.child_count = 0;

    fs->root = &ramfs_root.vnode;

    fs->ops = &ramfs_fs_ops;

    return KERNEL_SUCCESS;
}

int ramfs_lookup(vnode_t *dir, const char *name, vnode_t **result)
{
    if (dir == NULL || name == NULL || result == NULL)
        return KERNEL_EINVAL;

    ramfs_node_t *node = (ramfs_node_t *)dir->private_data;

    if (node == NULL)
    {
        return KERNEL_EINVAL;
    }

    for (uint32_t i = 0; i < node->child_count; i++)
    {
        ramfs_node_t *child = node->children[i];

        if (child == NULL)
        {
            continue;
        }

        if (k_strcmp(child->vnode.name, name) == 0)
        {
            *result = &child->vnode;
            return KERNEL_SUCCESS;
        }
    }

    return KERNEL_ENOENT;

}

int ramfs_mkdir(vnode_t *dir, const char *name)
{
    if (dir == NULL || name == NULL)
        return KERNEL_EINVAL;

    if (dir->type != VNODE_DIRECTORY)
    {
        return KERNEL_ENOTSUP;
    }

    ramfs_node_t *parent = (ramfs_node_t *)dir->private_data;

    if (parent == NULL)
    {
        return KERNEL_EINVAL;
    }

    vnode_t *existing;

    if (ramfs_lookup(dir, name, &existing) == KERNEL_SUCCESS)
    {
        return KERNEL_EEXIST;
    }

    if (parent->child_count >= RAMFS_MAX_CHILDREN)
    {
        return KERNEL_ENOMEM;
    }

    ramfs_node_t *child = kmalloc(sizeof(ramfs_node_t));

    if (child == NULL)
    {
        return KERNEL_ENOMEM;
    }

    k_memset(child, 0 , sizeof(ramfs_node_t));

    child->vnode.type = VNODE_DIRECTORY;

    k_strcpy(child->vnode.name, name);

    child->vnode.size = 0;

    child->vnode.parent = dir;

    child->vnode.fs = dir->fs;

    child->vnode.ops = &ramfs_vnode_ops;

    child->vnode.private_data = child;

    child->child_count = 0;

    child->data = NULL;

    child->capacity = 0;

    child->vnode.size = 0;

    parent->children[parent->child_count++] = child;

    return KERNEL_SUCCESS;
}

int ramfs_create(vnode_t *dir, const char *name)
{
    if (dir == NULL || name == NULL)
        return KERNEL_EINVAL;

    if (dir->type != VNODE_DIRECTORY)
    {
        return KERNEL_ENOTSUP;
    }

    ramfs_node_t *parent = (ramfs_node_t *)dir->private_data;

    if (parent == NULL)
    {
        return KERNEL_EINVAL;
    }

    vnode_t *existing;

    if (ramfs_lookup(dir, name, &existing) == KERNEL_SUCCESS)
        return KERNEL_EEXIST;

    if (parent->child_count >= RAMFS_MAX_CHILDREN)
        return KERNEL_ENOMEM;

    ramfs_node_t *child = kmalloc(sizeof(ramfs_node_t));

    if (child == NULL)
        return KERNEL_ENOMEM;

    k_memset(child, 0, sizeof(ramfs_node_t));

    child->data = NULL;

    child->capacity = 0;

    child->vnode.type = VNODE_FILE;

    k_strcpy(child->vnode.name, name);

    child->vnode.size = 0;

    child->vnode.parent = dir;

    child->vnode.fs = dir->fs;

    child->vnode.ops = &ramfs_vnode_ops;

    child->vnode.private_data = child;

    child->child_count = 0;

    child->vnode.size = 0;

    parent->children[parent->child_count++] = child;

    return KERNEL_SUCCESS;

}

int ramfs_write(file_t *file, const void *buffer, uint64_t size)
{
    if (file == NULL || buffer == NULL)
        return KERNEL_EINVAL;

    ramfs_node_t *node = (ramfs_node_t *)file->node->private_data;

    if (node == NULL)
    {
        return KERNEL_EINVAL;
    }

    if (file->node->type != VNODE_FILE)
        return KERNEL_ENOTSUP;

    if (node->data == NULL)
    {
        node->data = kmalloc(size);

        if (node->data == NULL)
            return KERNEL_ENOMEM;

        node->capacity = size;
    }

    k_memcpy(node->data, buffer, size);

    node->vnode.size = size;

    file->position = size;

    return KERNEL_SUCCESS;

}

int ramfs_read(file_t *file, void *buffer, uint64_t size)
{
    if (file == NULL || buffer == NULL)
        return KERNEL_EINVAL;

    ramfs_node_t *node = (ramfs_node_t *)file->node->private_data;

    if (node == NULL)
        return KERNEL_EINVAL;

    if (file->node->type != VNODE_FILE)
        return KERNEL_ENOTSUP;

    if (node->data == NULL)
        return KERNEL_SUCCESS;

    uint64_t bytes = size;

    if (bytes > node->vnode.size)
        bytes = node->vnode.size;

    k_memcpy(buffer, node->data, bytes);

    file->position += bytes;

    return (int)bytes;
}

int ramfs_init(filesystem_t *fs)
{
    if (fs == NULL)
    {
        return KERNEL_EINVAL;
    }

    k_strcpy(fs->name, "ramfs");

    fs->root = NULL;

    fs->ops = &ramfs_fs_ops;

    return KERNEL_SUCCESS;
}

