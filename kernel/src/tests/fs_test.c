#include "fs_test.h"

#include "../fs/vfs.h"
#include "../fs/ramfs.h"

#include "../stdio/printf.h"
#include "../kernel_err/errno.h"
#include "../lib/memory.h"
#include "../lib/string.h"

#include <stddef.h>

static filesystem_t root_fs;

void fs_test(void)
{
    kprintf("\n========== Filesystem Test ==========\n");

    vfs_init();

    if (ramfs_init(&root_fs) != KERNEL_SUCCESS)
    {
        kprintf("[FAIL] ramfs_init()\n");
        return;
    }

    if (vfs_mount(&root_fs, NULL) != KERNEL_SUCCESS)
    {
        kprintf("[FAIL] vfs_mount()\n");
        return;
    }

    kprintf("[PASS] Mounted RAMFS\n");

    vnode_t *root = vfs_get_root();

    if (root == NULL)
    {
        kprintf("[FAIL] Root vnode is NULL\n");
        return;
    }

    kprintf("[PASS] Root vnode acquired\n");

    ramfs_mkdir(root, "dev");
    ramfs_mkdir(root, "home");
    ramfs_mkdir(root, "tmp");

    vnode_t *home;

    if (vfs_lookup("/home", &home) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /home\n");
    else
    {
        kprintf("[FAIL] Lookup /home\n");
        return;
    }

    if (ramfs_mkdir(home, "divyanshu") == KERNEL_SUCCESS)
        kprintf("[PASS] mkdir divyanshu\n");
    else
        kprintf("[FAIL] mkdir divyanshu\n");

    vnode_t *user;

    if (vfs_lookup("/home/divyanshu", &user) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /home/divyanshu\n");
    else
    {
        kprintf("[FAIL] Lookup /home/divyanshu\n");
        return;
    }

    if (ramfs_mkdir(user, "projects") == KERNEL_SUCCESS)
        kprintf("[PASS] mkdir projects\n");
    else
        kprintf("[FAIL] mkdir projects\n");

    vnode_t *projects;

    if (vfs_lookup("/home/divyanshu/projects", &projects) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /home/divyanshu/projects\n");
    else
    {
        kprintf("[FAIL] Lookup /home/divyanshu/projects\n");
        return;
    }

    /* Create notes.txt inside /home/divyanshu/projects */

    if (ramfs_create(projects, "notes.txt") == KERNEL_SUCCESS)
        kprintf("[PASS] create notes.txt\n");
    else
    {
        kprintf("[FAIL] create notes.txt\n");
        return;
    }

    vnode_t *node;

    if (vfs_lookup("/dev", &node) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /dev\n");
    else
        kprintf("[FAIL] Lookup /dev\n");

    if (vfs_lookup("/tmp", &node) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /tmp\n");
    else
        kprintf("[FAIL] Lookup /tmp\n");

    if (vfs_lookup("/home/divyanshu/projects", &node) == KERNEL_SUCCESS)
        kprintf("[PASS] Deep lookup\n");
    else
        kprintf("[FAIL] Deep lookup\n");

    int rc = vfs_lookup("/does_not_exist", &node);

    if (rc == KERNEL_ENOENT)
        kprintf("[PASS] Missing file detection\n");
    else
        kprintf("[FAIL] Missing file detection\n");

    file_t *file;

    if (vfs_open("/home/divyanshu/projects/notes.txt", &file) == KERNEL_SUCCESS)
    {
        kprintf("[PASS] open notes.txt\n");
    }
    else
    {
        kprintf("[FAIL] open notes.txt\n");
        return;
    }

    const char *text = "Hello ZenithOS";

    if (vfs_write(file, text, 15) == KERNEL_SUCCESS)
    {
        kprintf("[PASS] write notes.txt\n");
    }
    else
    {
        kprintf("[FAIL] write notes.txt\n");
        return;
    }

    ramfs_node_t *ramnode = (ramfs_node_t *)file->node->private_data;

    if (ramnode == NULL)
    {
        kprintf("[FAIL] private_data\n");
        return;
    }

    if (ramnode->data == NULL)
    {
        kprintf("[FAIL] Data Buffer\n");
        return;
    }

    kprintf("Stored Data: %s\n", (char *)ramnode->data);
    kprintf("File Size: %llu\n", (unsigned long long)ramnode->vnode.size);
    kprintf("Capacity: %llu\n", (unsigned long long)ramnode->capacity);

    char buffer[32];

    k_memset(buffer, 0, sizeof(buffer));

    int bytes = vfs_read(file, buffer, sizeof(buffer));

    if (bytes == 15 && k_strcmp(buffer, "Hello ZenithOS") == 0)
    {
        kprintf("[PASS] read notes.txt\n");
    }
    else
    {
        kprintf("[FAIL] read notes.txt\n");
        kprintf("Bytes : %d\n", bytes);
        kprintf("Data  : %s\n", buffer);
    }

    kprintf("========== Filesystem Test Complete ==========\n");
}