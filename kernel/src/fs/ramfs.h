#pragma once

#include "vnode.h"
#include "filesystem.h"
#include "file.h"

#define RAMFS_MAX_CHILDREN  64

typedef struct ramfs_node
{
    vnode_t vnode;

    struct ramfs_node *children[RAMFS_MAX_CHILDREN];

    uint32_t child_count;

    void *data;

    uint64_t capacity;

} ramfs_node_t;

int ramfs_mount(filesystem_t *fs);

int ramfs_lookup(vnode_t *dir, const char *name, vnode_t **result);

int ramfs_create(vnode_t *dir, const char *name);

int ramfs_mkdir(vnode_t *dir, const char *name);

int ramfs_write(file_t *file, const void *buffer, uint64_t size);

int ramfs_read(file_t *file, void *buffer, uint64_t size);

int ramfs_init(filesystem_t *fs);



