#include "elf_loader.h"

#include "../stdio/printf.h"
#include "../kernel_err/errno.h"
#include "../vmm/user_vm_layout.h"
#include "../lib/memory.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

int elf_dump_segments(const elf64_header_t *header, size_t image_len)
{
    if (header == NULL || elf_validate(header, image_len) != KERNEL_SUCCESS)
        return KERNEL_EINVAL;

    kprintf("\n====== PROGRAM HEADERS ======\n");

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i, image_len);
        if (ph == NULL)
            continue;

        kprintf("Segment %u: Type=%u Offset=%llu VAddr=0x%llx FileSz=%llu MemSz=%llu Flags=0x%x\n",
                i, ph->p_type, (unsigned long long)ph->p_offset,
                (unsigned long long)ph->p_vaddr, (unsigned long long)ph->p_filesz,
                (unsigned long long)ph->p_memsz, ph->p_flags);
    }
    return KERNEL_SUCCESS;
}

int elf_load_image(const void *image, size_t image_len, elf_image_t *loaded)
{
    if (image == NULL || loaded == NULL)
        return KERNEL_EINVAL;

    const elf64_header_t *header = (const elf64_header_t *)image;

    int status = elf_validate(header, image_len);
    if (status != KERNEL_SUCCESS)
        return status;

    loaded->base = NULL;
    loaded->size = 0;
    loaded->entry = header->e_entry;
    loaded->image_base = 0;

    uint64_t lowest = UINT64_MAX;
    uint64_t highest = 0;
    bool has_exec_segment = false;
    bool entry_in_exec_segment = false;

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i, image_len);
        if (ph == NULL)
            return KERNEL_EINVAL;

        if (ph->p_type != PT_LOAD)
            continue;

        // Bounded file offsets with overflow check
        if (ph->p_offset > image_len ||
            ph->p_filesz > image_len ||
            ph->p_offset + ph->p_filesz < ph->p_offset ||
            ph->p_offset + ph->p_filesz > image_len)
        {
            return KERNEL_EINVAL;
        }

        // Memory size must be >= file size
        if (ph->p_filesz > ph->p_memsz)
        {
            return KERNEL_EINVAL;
        }

        // Must be in canonical user space with overflow check
        if (ph->p_vaddr < USER_BASE || ph->p_vaddr >= USER_MAX_ADDRESS)
        {
            return KERNEL_EINVAL;
        }

        uint64_t end = ph->p_vaddr + ph->p_memsz;
        if (end < ph->p_vaddr || end > USER_MAX_ADDRESS)
        {
            return KERNEL_EINVAL;
        }

        // Verify alignment if specified
        if (ph->p_align > 1)
        {
            if ((ph->p_align & (ph->p_align - 1)) != 0)
            {
                return KERNEL_EINVAL;
            }
        }

        if (ph->p_flags & PF_X)
        {
            has_exec_segment = true;
            if (header->e_entry >= ph->p_vaddr && header->e_entry < end)
            {
                entry_in_exec_segment = true;
            }
        }

        if (ph->p_vaddr < lowest)
            lowest = ph->p_vaddr;

        if (end > highest)
            highest = end;
    }

    if (!has_exec_segment || !entry_in_exec_segment)
    {
        return KERNEL_EINVAL;
    }

    if (lowest != UINT64_MAX && highest >= lowest)
    {
        loaded->size = highest - lowest;
        loaded->image_base = lowest;
    }

    return KERNEL_SUCCESS;
}
