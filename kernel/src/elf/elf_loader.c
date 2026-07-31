#include "elf_loader.h"

#include "../stdio/printf.h"
#include "../kernel_err/errno.h"
#include "../mm/heap.h"
#include "../lib/memory.h"

#include <stdint.h>
#include <stddef.h>

int elf_dump_segments(const elf64_header_t *header)
{
    if (elf_validate(header) != KERNEL_SUCCESS)
        return KERNEL_EINVAL;

    kprintf("\n====== PROGRAM HEADERS ======\n");

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i);

        if (ph == NULL)
            continue;

        kprintf("\nSegment %u\n", i);

        kprintf("Type    : %u\n", ph->p_type);
        kprintf("Offset  : %llu\n", (unsigned long long)ph->p_offset);

        kprintf("VAddr   : 0x%llx\n", (unsigned long long)ph->p_vaddr);

        kprintf("FileSz  : %llu\n", (unsigned long long)ph->p_filesz);

        kprintf("MemSz  : %llu\n", (unsigned long long)ph->p_memsz);

        kprintf("Flags   : 0x%x\n", ph->p_flags);
    }
    return KERNEL_SUCCESS;
}

int elf_load_image(const void *image, elf_image_t *loaded)
{
    if (image == NULL || loaded == NULL)
        return KERNEL_EINVAL;

    const elf64_header_t *header = (const elf64_header_t *)image;

    if (elf_validate(header) != KERNEL_SUCCESS)
        return KERNEL_EINVAL;

    loaded->base = NULL;
    loaded->size = 0;
    loaded->entry = header->e_entry;
    loaded->image_base = 0;

    uint64_t lowest = UINT64_MAX;
    uint64_t highest = 0;

    kprintf("\n====== ELF LOADER ======\n");

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i);

        if (ph == NULL)
            continue;

        if (ph->p_type != PT_LOAD)
            continue;

        if (ph->p_vaddr < lowest)
            lowest = ph->p_vaddr;

        uint64_t end = ph->p_vaddr + ph->p_memsz;

        if (end > highest)
            highest = end;

        kprintf("\nLOAD SEGMENT\n");

        kprintf("Offset : %llu\n", (unsigned long long)ph->p_offset);
        kprintf("VAddr  : 0x%llx\n", (unsigned long long)ph->p_vaddr);
        kprintf("FileSz : %llu\n", (unsigned long long)ph->p_filesz);
        kprintf("MemSz  : %llu\n", (unsigned long long)ph->p_memsz);
    }

    if (lowest != UINT64_MAX)
    {
        loaded->size = highest - lowest;
        loaded->image_base = lowest;
        loaded->base = kmalloc(loaded->size);

        if (loaded->base == NULL)
        {
            kprintf("FAIL] Image allocation\n");
            return KERNEL_ENOMEM;
        }

        kprintf("\n====== IMAGE INFO ======\n");

        kprintf("Lowest : 0x%llx\n", (unsigned long long)lowest);
        kprintf("Highest: 0x%llx\n", (unsigned long long)highest);
        kprintf("Size   : %llu bytes\n", (unsigned long long)loaded->size);
        kprintf("Base   : 0x%llx\n", (unsigned long long)loaded->base);

    }

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        const elf64_program_header_t *ph = elf_program_header(header, i);

        if (ph == NULL)
            continue;

        if (ph->p_type != PT_LOAD)
            continue;

        uint64_t offset = ph->p_vaddr - lowest;
        uint8_t *dest = (uint8_t *)loaded->base + offset;
        uint8_t *src = (const uint8_t *)image + ph->p_offset;

        k_memcpy(dest, src, ph->p_filesz);

        if (ph->p_memsz > ph->p_filesz)
        {
            k_memset(dest + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
            kprintf("Zeroed %llu BSS bytes\n", (unsigned long long)(ph->p_memsz - ph->p_filesz));
        }

        kprintf("Copied %llu bytes to offset %llu\n", (unsigned long long)ph->p_filesz, (unsigned long long)offset);

    }
    return KERNEL_SUCCESS;
}