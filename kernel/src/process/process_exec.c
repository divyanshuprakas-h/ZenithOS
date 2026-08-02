#include "process_exec.h"
#include "process_loader.h"

#include "../elf/elf_loader.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../vmm/page_table.h"

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

    page_entry_t *pte = page_walk(process->page_table, process->user_rip, false);

    if (pte == NULL)
    {
        kprintf("[USER] No PTE for entry!\n");
        return KERNEL_EINVAL;
    }

    uint64_t phys = (*pte) & PAGE_TABLE_ADDRESS_MASK;

    uint8_t *code = (uint8_t *)page_table_physical_to_virtual(phys);

    kprintf("[USER CODE]\n");

    for (int i = 0; i < 8; i++)
    {
        kprintf("Byte %d = 0x%llx\n", i, (unsigned long long)code[i]);
    }

    process->user_rsp = process->user_stack_top - 8;
    process->user_rflags = 0x202;
    process->is_user_process = true;

    kprintf("[USER] RIP = 0x%llx\n", (unsigned long long)process->user_rip);
    kprintf("[USER] RSP = 0x%llx\n", (unsigned long long)process->user_rsp);
    kprintf("[USER] RFLAGS = 0x%llx\n", (unsigned long long)process->user_rflags);
    kprintf("[PROCESS] PID %llu marked as USER process\n", (unsigned long long)process->pid);

    return KERNEL_SUCCESS;
}