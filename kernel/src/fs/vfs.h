#pragma once

#include <stddef.h>
#include <stdint.h>
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

typedef struct {
    char name[64];
    vnode_type_t type;
    uint64_t size;
} vfs_dirent_t;

int vfs_read_dir(const char *path, vfs_dirent_t *entries, size_t max_entries, size_t *out_count);

int vfs_read(file_t *file, void *buffer, uint64_t size);

int vfs_close(file_t *file);

int vfs_create(const char *path);

int vfs_mkdir(const char *path);

int vfs_unlink(const char *path);


