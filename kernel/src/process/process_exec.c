#include "process_exec.h"
#include "process_loader.h"

#include "../elf/elf_loader.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../vmm/page_table.h"

#include <stddef.h>

int process_exec(process_t *process, const void *elf_image, size_t image_len)
{
    if (process == NULL || elf_image == NULL)
        return KERNEL_EINVAL;

    elf_image_t image;
    int status = elf_load_image(elf_image, image_len, &image);
    if (status != KERNEL_SUCCESS)
        return status;

    const elf64_header_t *header = (const elf64_header_t *)elf_image;

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i, image_len);
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

    page_entry_t *pte = page_walk(process->page_table, process->user_rip, false);
    if (pte == NULL || !page_is_present(*pte) || !page_is_user(*pte))
    {
        kprintf("[USER] Invalid PTE for entry point 0x%llx\n", (unsigned long long)process->user_rip);
        return KERNEL_EINVAL;
    }

    process->user_rsp = process->user_stack_top - 8;
    process->user_rflags = 0x202;
    process->is_user_process = true;

    kprintf("[PROCESS] PID %llu loaded ELF image (entry=0x%llx, rsp=0x%llx)\n",
            (unsigned long long)process->pid,
            (unsigned long long)process->user_rip,
            (unsigned long long)process->user_rsp);

    return KERNEL_SUCCESS;
}
