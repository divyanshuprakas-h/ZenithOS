#include "process_exec.h"
#include "process_loader.h"

#include "../elf/elf_loader.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"

#include <stddef.h>

int process_exec(process_t *process, const void *elf_image)
{
    if (process == NULL || elf_image == NULL)
        return KERNEL_EINVAL;

    elf_image_t image;

    int status = elf_load_image(elf_image, &image);

    if (status != KERNEL_SUCCESS)
        return status;

    const elf64_header_t *header = (const elf64_header_t *)elf_image;

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i);

        if (ph == NULL)
            continue;

        if (ph->p_type != PT_LOAD)
            continue;

        int rc = process_map_segment(process, ph, elf_image);

        if (rc != KERNEL_SUCCESS)
        {
            return rc;
        }
    }

    process->image = image;
    
    process->user_rip = image.entry;
    process->user_rsp = process->user_stack_top;
    process->user_rflags = 0x202;

    kprintf("[USER] RIP = 0x%llx\n", (unsigned long long)process->user_rip);
    kprintf("[USER] RSP = 0x%llx\n", (unsigned long long)process->user_rsp);
    kprintf("[USER] RFLAGS = 0x%llx\n", (unsigned long long)process->user_rflags);

    return KERNEL_SUCCESS;
}