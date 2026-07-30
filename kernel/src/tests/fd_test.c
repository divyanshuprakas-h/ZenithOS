#include "fs_test.h"

#include "../kernel_err/errno.h"
#include "../fs/fd.h"
#include "../stdio/printf.h"

#include <stddef.h>

void fd_test(void)
{
    fd_table_t table;

    fd_init(&table);

    file_t file;

    int fd = fd_allocate(&table, &file);

    if (fd == 0)
        kprintf("[PASS] fd_allocate\n");
    else
        kprintf("[FAIL] fd_allocate\n");

    file_t *result = fd_get(&table, fd);

    if (result == &file)
        kprintf("[PASS] fd_get\n");
    else
        kprintf("[FAIL] fd_get\n");

    if (fd_close(&table, fd) == KERNEL_SUCCESS)
        kprintf("[PASS] fd_close\n");
    else
        kprintf("[FAIL] fd_close\n");

    if (fd_get(&table, fd) == NULL)
        kprintf("[PASS] fd released\n");
    else
        kprintf("[FAIL] fd released\n");
}