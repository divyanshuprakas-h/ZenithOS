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
struct file;

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


typedef struct vnode_operations
{
    int (*lookup)(struct vnode *dir,
                const char *name,
                struct vnode **result);

    int (*create)(struct vnode *dir,
                const char *name);

    int (*mkdir)(struct vnode *dir,
                const char *name);

    int (*read)(struct file *file,
                void *buffer,
                uint64_t size);

    int (*write)(struct file *file,
                const void *buffer,
                uint64_t size);

    int (*remove)(struct vnode *node);

} vnode_operations_t;
