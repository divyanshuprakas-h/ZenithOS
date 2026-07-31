#include "process_exec_file.h"
#include "process_exec.h"

#include "../fs/vfs.h"
#include "../fs/file.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../mm/heap.h"
#include "../lib/memory.h"

#include <stddef.h>

int process_exec_file(process_t *process, const char *path)
{
    if (process == NULL || path == NULL)
        return KERNEL_EINVAL;

    kprintf("[EXEC] STEP 1\n");

    file_t *file;

    int open_status = vfs_open(path, &file);
    kprintf("[EXEC] STEP 2 file=%p status=%d\n", file, open_status);

    if (open_status != KERNEL_SUCCESS)
    {
        kprintf("[EXEC] Failed to open %s (%d)\n", path, open_status);
        return open_status;
    }

    kprintf("[EXEC] Opened %s\n", path);
    kprintf("[EXEC] STEP 3\n");

    #define EXEC_BUFFER_SIZE 65536

    void *buffer = kmalloc(EXEC_BUFFER_SIZE);

    if (buffer == NULL)
    {
        file_close(file);
        return KERNEL_ENOMEM;
    }

    k_memset(buffer, 0, EXEC_BUFFER_SIZE);

    int bytes = vfs_read(file, buffer, EXEC_BUFFER_SIZE);

    if (bytes < 0)
    {
        kprintf("[EXEC] Read failed (%d)\n", bytes);

        kfree(buffer);
        file_close(file);

        return bytes;
    }

    kprintf("[EXEC] Read %d bytes\n", bytes);
    kprintf("[EXEC] STEP 4 bytes=%d\n", bytes);

    int status = process_exec(process, buffer);

    if (status != KERNEL_SUCCESS)
    {
        kprintf("[EXEC] process_exec() failed (%d)\n", status);

        kfree(buffer);
        file_close(file);

        return status;
    }

    kprintf("[EXEC] Image loaded successfully\n");
    kprintf("[EXEC] STEP 5 status=%d\n", status);

    file_close(file);
    kprintf("[EXEC] Closed file\n");
    kprintf("[EXEC] STEP 6\n");
    kfree(buffer);

    return KERNEL_SUCCESS;
}
