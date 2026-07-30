#pragma once

#include "vnode.h"

#include <stdint.h>

struct vnode;

typedef struct file
{
    struct vnode *node;

    uint64_t position;

    uint32_t flags;

    uint32_t refcount;

} file_t;

int file_open(vnode_t *node, file_t **result);

void file_close(file_t *file);

