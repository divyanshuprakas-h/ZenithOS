#include "../process/process.h"
#include "../stdio/printf.h"
#include "../fs/fd.h"
#include "../fs/file.h"
#include "../fs/sys_file.h"
#include "../kernel_err/errno.h"
#include "../lib/memory.h"
#include "../process/process_exec.h"
#include "../elf/elf_loader.h"

#include <stddef.h>


// void process_test(void)
// {
//     process_t *current = process_current();

//     if (current == NULL)
//     {
//         kprintf("ERROR: process_current() returned NULL!\n");
//     }
//     else
//     {
//         kprintf("Current PID: %llu\n", (unsigned long long)current->pid);
//         kprintf("Current Name: %s\n", current->name);
//     }
//     while (1)
//     {
//         scheduler_yield();
//     }
// }

void process_test(void)
{
    process_t *current = process_current();

    kprintf("Process %llu started\n", (unsigned long long)current->pid);

    int fd = sys_open("/home/divyanshu/projects/notes.txt");

    if (fd < 0)
    {
        kprintf("[FAIL] sys_open (%d)\n", fd);
        process_exit(42);
        return;
    }

    kprintf("[PASS] sys_open\n");
    kprintf("FD = %d\n", fd);

    const char *msg = "Hello from Process";

    int written = sys_write(fd, msg, 18);

    if (written >= 0)
        kprintf("[PASS] sys_write\n");
    else
        kprintf("[FAIL] sys_write (%d)\n", written);

    char buffer[64];

    k_memset(buffer, 0, sizeof(buffer));

    int bytes = sys_read(fd, buffer, sizeof(buffer));

    if (bytes >= 0)
    {
        kprintf("[PASS] sys_read\n");
        kprintf("Read: %s\n", buffer);
    }
    else
    {
        kprintf("[FAIL] sys_read (%d)\n", bytes);
    }

    if (sys_close(fd) == KERNEL_SUCCESS)
        kprintf("[PASS] sys_close\n");
    else       
        kprintf("[FAIL] sys_close\n");

    process_exit(42);

}

void test_process_void(void)
{

    kprintf("==============PROCESS===================\n");

    process_t *p1 = process_create("Process 1", process_test);
    process_t *p2 = process_create("Process 2", process_test);
    process_t *p3 = process_create("Process 3", process_test);

    kprintf("\n===== PROCESS FD TABLE TEST =====\n");

    file_t file1;
    file_t file2;
    file_t file3;

    int fd1 = fd_allocate(&p1->fd_table, &file1);
    int fd2 = fd_allocate(&p2->fd_table, &file2);
    int fd3 = fd_allocate(&p3->fd_table, &file3);

    kprintf("P1 FD = %d\n", fd1);
    kprintf("P2 FD = %d\n", fd2);
    kprintf("P3 FD = %d\n", fd3);

    if (fd1 == 0 && fd2 == 0 && fd3 == 0)
        kprintf("[PASS] Independent FD tables\n");
    else
        kprintf("[FAIL] Independent FD tables\n");

    if (fd_get(&p1->fd_table, fd1) == &file1)
        kprintf("[PASS] P1 FD lookup\n");
    else
        kprintf("[FAIL] P1 FD lookup\n");

    if (fd_get(&p2->fd_table, fd2) == &file2)
        kprintf("[PASS] P2 FD lookup\n");
    else
        kprintf("[FAIL] P2 FD lookup\n");

    if (fd_get(&p3->fd_table, fd3) == &file3)
        kprintf("[PASS] P3 FD lookup\n");
    else
        kprintf("[FAIL] P3 FD lookup\n");

    kprintf("=================================\n");
    
    kprintf("P1 PID = %llu\n", (unsigned long long)p1->pid);
    kprintf("P2 PID = %llu\n", (unsigned long long)p2->pid);
    kprintf("P3 PID = %llu\n", (unsigned long long)p3->pid);

    kprintf("\n===== PROCESS LOOKUP TEST =====\n");

    process_t *lookup = process_lookup(2);

    if (lookup != NULL)
    {
        kprintf("Lookup Success!\n");
        kprintf("PID  : %llu\n", (unsigned long long)lookup->pid);
        kprintf("Name : %s\n", lookup->name);
    }
    else
    {
        kprintf("Lookup Failed\n");
    }

    kprintf("\n===== INVALID LOOKUP TEST =====\n");

    process_t *invalid = process_lookup(999);

    if (invalid == NULL)
    {
        kprintf("PASS: Invalid PID returned NULL\n");
    }
    else
    {
        kprintf("FAIL: Invalid PID should not exist!\n");
    }

    kprintf("=========================================\n");

    kprintf("\n===== PROCESS IMAGE TEST =====\n");

    elf_image_t image;

    k_memset(&image, 0, sizeof(image));

    image.base = (void *)0x100000;
    image.size = 8192;
    image.image_base = 0x400000;
    image.entry = 0x401000;

    if (process_load_image(p1, &image) == KERNEL_SUCCESS)
    {
        kprintf("[PASS] process_load_image\n");

        kprintf("Image Base : 0x%llx\n", (unsigned long long)p1->image.image_base);
        kprintf("Entry      : 0x%llx\n", (unsigned long long)p1->image.entry);
        kprintf("Image Size : %llu\n", (unsigned long long)p1->image.size);
    }
    else
    {
        kprintf("[FAIL] process_load_image\n");
    }

    kprintf("=========================================\n");

}
