#pragma once

#include "vnode.h"
#include "file.h"

struct filesystem;
struct vnode;

void vfs_init(void);

int vfs_mount(struct filesystem *fs, 
            struct vnode *mountpoint);

struct vnode *vfs_get_root(void);

int vfs_lookup(const char *path, vnode_t **result);

int vfs_open(const char *path, file_t **result);

int vfs_write(file_t *file, const void *buffer, uint64_t size);

int vfs_read(file_t *file, void *buffer, uint64_t size);

