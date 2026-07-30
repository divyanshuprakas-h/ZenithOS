#pragma once 

struct vnode;
struct filesystem_operations;

typedef struct filesystem_operations
{
    int (*mount)(struct filesystem *fs);

    int (*unmount)(struct filesystem *fs);

    int (*sync)(struct filesystem *fs);

} filesystem_operations_t;

typedef struct filesystem
{
    char name[32];

    struct vnode *root;

    struct filesystem_operations *ops;

} filesystem_t;