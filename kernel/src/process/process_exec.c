#include "process_exec.h"

#include "../elf/elf_loader.h"
#include "../kernel_err/errno.h"

#include <stddef.h>

int process_exec(process_t *process, const void *elf_image)
{
    if (process == NULL || elf_image == NULL)
        return KERNEL_EINVAL;

    elf_image_t image;

    int status = elf_load_image(elf_image, &image);

    if (status != KERNEL_SUCCESS)
        return status;

    process->image = image;

    return KERNEL_SUCCESS;
}