#pragma once

#include <stdint.h>

typedef enum
{
    VNODE_FILE,
    VNODE_DIRECTORY,
    VNODE_DEVICE,
    VNODE_SYMLINK
} vnode_type_t;

struct vnode;
struct filesystem;
struct vnode_operations;

typedef struct vnode
{
    char name[64];

    vnode_type_t type;

    uint64_t size;

    struct vnode *parent;

    struct filesystem *fs;

    struct vnode_operations *ops;

    void *private_data;

} vnode_t;

