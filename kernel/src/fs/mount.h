#pragma once 

#include <stdint.h>

struct filesystem;

struct vnode;

typedef struct mount 
{
    struct filesystem *fs;

    struct vnode *mountpoint;

    struct vnode *root;

    uint32_t flags;

    struct mount *parent;

    struct mount *next;
    
} mount_t;

