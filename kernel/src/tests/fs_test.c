#include "fs_test.h"

#include "../elf/elf_types.h"
#include "../fs/file.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"

#include "../stdio/printf.h"
#include "../kernel_err/errno.h"
#include "../lib/memory.h"
#include "../lib/string.h"

#include <stddef.h>

static filesystem_t root_fs;

typedef struct
{
    elf64_header_t header;
    elf64_program_header_t program_header;
    uint8_t payload[16];
} init_elf_image_t;

static int write_init_elf(file_t *file)
{
    if (file == NULL)
    {
        return KERNEL_EINVAL;
    }

    init_elf_image_t image;

    k_memset(&image, 0, sizeof(image));

    image.header.e_ident[0] = 0x7F;
    image.header.e_ident[1] = 'E';
    image.header.e_ident[2] = 'L';
    image.header.e_ident[3] = 'F';
    image.header.e_ident[4] = ELFCLASS64;
    image.header.e_ident[5] = ELFDATA2LSB;
    image.header.e_version = EV_CURRENT;
    image.header.e_type = ET_EXEC;
    image.header.e_machine = EM_X86_64;
    image.header.e_entry = 0x400000;
    image.header.e_phoff = sizeof(elf64_header_t);
    image.header.e_phentsize = sizeof(elf64_program_header_t);
    image.header.e_phnum = 1;

    image.program_header.p_type = PT_LOAD;
    image.program_header.p_flags = PF_R | PF_X;
    image.program_header.p_offset = offsetof(init_elf_image_t, payload);
    image.program_header.p_vaddr = 0x400000;
    image.program_header.p_filesz = sizeof(image.payload);
    image.program_header.p_memsz = sizeof(image.payload);
    image.program_header.p_align = 0x1000;

    image.payload[0] = 0xC3;

    for (uint64_t i = 1; i < sizeof(image.payload); i++)
    {
        image.payload[i] = 0x90;
    }

    return vfs_write(
        file,
        &image,
        offsetof(init_elf_image_t, payload) + sizeof(image.payload));
}

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
    ramfs_mkdir(root, "bin");

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

    if (vfs_lookup("/bin", &node) == KERNEL_SUCCESS)
        kprintf("[PASS] Lookup /bin\n");
    else
        kprintf("[FAIL] Lookup /bin\n");

    if (vfs_lookup("/home/divyanshu/projects", &node) == KERNEL_SUCCESS)
        kprintf("[PASS] Deep lookup\n");
    else
        kprintf("[FAIL] Deep lookup\n");

    vnode_t *bin;

    if (vfs_lookup("/bin", &bin) == KERNEL_SUCCESS)
    {
        kprintf("PASS] Lookup /bin\n");
    }
    else
    {
        kprintf("[FAIL] Lookup /bin\n");
        return;
    }

    if (ramfs_create(bin, "init") == KERNEL_SUCCESS)
    {
        kprintf("[PASS] create /bin/init\n");
    }
    else
    {
        kprintf("[FAIL] create /bin/init\n");
        return;
    }

    vnode_t *init_node;

    int rc = vfs_lookup("/bin/init", &init_node);

    kprintf("Lookup rc = %d\n", rc);

    if (rc == KERNEL_SUCCESS)
    {
        kprintf("[PASS] Lookup /bin/init\n");
    }
    else
    {
        kprintf("[FAIL] Lookup /bin/init\n");
        return;
    }

    file_t *test_file;

    rc = file_open(init_node, &test_file);

    kprintf("file_open rc = %d\n", rc);

    if (rc == KERNEL_SUCCESS)
    {
        kprintf("[PASS] file_open\n");
    }
    else
    {
        kprintf("[FAIL] file_open\n");
    }

    rc = write_init_elf(test_file);

    if (rc == KERNEL_SUCCESS)
    {
        kprintf("[PASS] write /bin/init ELF\n");
    }
    else
    {
        kprintf("[FAIL] write /bin/init ELF (%d)\n", rc);
        return;
    }

    file_close(test_file);

    rc = vfs_lookup("/does_not_exist", &node);

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
