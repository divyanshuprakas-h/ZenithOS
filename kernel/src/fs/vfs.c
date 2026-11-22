#include "vfs.h"
#include "mount.h"
#include "filesystem.h"
#include "vnode.h"
#include "ramfs.h"
#include "path.h"

#include "kernel_err/errno.h"
#include "../mm/heap.h"
#include "../lib/string.h"

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

int vfs_read_dir(const char *path, vfs_dirent_t *entries, size_t max_entries, size_t *out_count)
{
    if (path == NULL || entries == NULL || out_count == NULL || max_entries == 0)
    {
        return KERNEL_EINVAL;
    }

    *out_count = 0;

    vnode_t *dir = NULL;
    int rc = vfs_lookup(path, &dir);
    if (rc != KERNEL_SUCCESS)
    {
        return rc;
    }

    if (dir->type != VNODE_DIRECTORY)
    {
        return KERNEL_ENOTSUP;
    }

    ramfs_node_t *rnode = (ramfs_node_t *)dir->private_data;
    if (rnode == NULL)
    {
        return KERNEL_EINVAL;
    }

    size_t count = 0;
    for (uint32_t i = 0; i < rnode->child_count && count < max_entries; i++)
    {
        ramfs_node_t *child = rnode->children[i];
        if (child == NULL)
            continue;

        k_strcpy(entries[count].name, child->vnode.name);
        entries[count].type = child->vnode.type;
        entries[count].size = child->vnode.size;
        count++;
    }

    *out_count = count;
    return KERNEL_SUCCESS;
}

int vfs_close(file_t *file)
{
    if (file == NULL)
        return KERNEL_EINVAL;

    file_close(file);
    return KERNEL_SUCCESS;
}

static int split_parent_and_name(const char *path, char *parent_path, size_t max_parent, const char **out_name)
{
    if (path == NULL || parent_path == NULL || out_name == NULL)
        return KERNEL_EINVAL;

    size_t len = k_strlen(path);
    while (len > 0 && path[len - 1] == '/')
    {
        len--;
    }
    if (len == 0)
    {
        return KERNEL_EINVAL;
    }

    int last_slash = -1;
    for (int i = (int)len - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        {
            last_slash = i;
            break;
        }
    }

    if (last_slash < 0)
    {
        k_strcpy(parent_path, "/");
        *out_name = path;
    }
    else if (last_slash == 0)
    {
        k_strcpy(parent_path, "/");
        *out_name = path + 1;
    }
    else
    {
        if ((size_t)last_slash >= max_parent)
            return KERNEL_EINVAL;
        for (int i = 0; i < last_slash; i++)
        {
            parent_path[i] = path[i];
        }
        parent_path[last_slash] = '\0';
        *out_name = path + last_slash + 1;
    }

    return KERNEL_SUCCESS;
}

int vfs_create(const char *path)
{
    char parent[128];
    const char *name = NULL;
    int rc = split_parent_and_name(path, parent, sizeof(parent), &name);
    if (rc != KERNEL_SUCCESS)
        return rc;

    vnode_t *dir = NULL;
    rc = vfs_lookup(parent, &dir);
    if (rc != KERNEL_SUCCESS)
        return rc;

    if (dir->ops == NULL || dir->ops->create == NULL)
        return KERNEL_ENOTSUP;

    return dir->ops->create(dir, name);
}

int vfs_mkdir(const char *path)
{
    char parent[128];
    const char *name = NULL;
    int rc = split_parent_and_name(path, parent, sizeof(parent), &name);
    if (rc != KERNEL_SUCCESS)
        return rc;

    vnode_t *dir = NULL;
    rc = vfs_lookup(parent, &dir);
    if (rc != KERNEL_SUCCESS)
        return rc;

    if (dir->ops == NULL || dir->ops->mkdir == NULL)
        return KERNEL_ENOTSUP;

    return dir->ops->mkdir(dir, name);
}

int vfs_unlink(const char *path)
{
    if (path == NULL)
        return KERNEL_EINVAL;

    vnode_t *node = NULL;
    int rc = vfs_lookup(path, &node);
    if (rc != KERNEL_SUCCESS)
        return rc;

    if (node->parent == NULL)
        return KERNEL_EBUSY;

    if (node->ops == NULL || node->ops->remove == NULL)
        return KERNEL_ENOTSUP;

    return node->ops->remove(node);
}
