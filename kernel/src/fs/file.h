#pragma once

#include <stdint.h>

struct vnode;

typedef struct file
{
    struct vnode *node;

    uint64_t position;

    uint32_t flags;

} file_t;

