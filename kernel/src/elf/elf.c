#include "elf.h"

#include "../kernel_err/errno.h"
#include "../stdio/printf.h"

#include <stddef.h>

int elf_validate(const elf64_header_t *header)
{
    if (header == NULL)
        return KERNEL_EINVAL;

    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F')
    {
        return KERNEL_EINVAL;
    }
        
    if (header->e_ident[4] != ELFCLASS64)
        return KERNEL_EINVAL;

    if (header->e_ident[5] != ELFDATA2LSB)
        return KERNEL_EINVAL;

    if (header->e_version != EV_CURRENT)
        return KERNEL_EINVAL;

    if (header->e_type != ET_EXEC)
        return KERNEL_EINVAL;

    if (header->e_machine != EM_X86_64)
        return KERNEL_EINVAL;

    return KERNEL_SUCCESS;
}

void elf_print_header(const elf64_header_t *header)
{
    kprintf("========== ELF ==========\n");

    kprintf("Entry          : 0x%llx\n", (unsigned long long)header->e_entry);
    kprintf("PH Offset      : %llu\n", (unsigned long long)header->e_phoff);
    kprintf("PH Count       : %u\n", header->e_phnum);
    kprintf("SH Offset      : %llu\n", (unsigned long long)header->e_shoff);
    kprintf("SH Count       : %u\n", header->e_shnum);

    kprintf("=========================\n");

}

const elf64_program_header_t *elf_program_header_t(const elf64_header_t *header, uint16_t index)
{
    if (header == NULL)
        return NULL;
    
    if (index >= header->e_phnum)
        return NULL;

    return (elf64_program_header_t *)((const uint8_t *)header + header->e_phoff + index * header->e_phentsize);
    
}

void elf_print_program_header(const elf64_program_header_t *ph)
{
    kprintf(
        "Type=%u"
        "Offset=%llu"
        "VAddr=0x%llx"
        "File=%llu"
        "Mem=%llu\n",

        ph->p_type,

        (unsigned long long)ph->p_offset,

        (unsigned long long)ph->p_vaddr,
        
        (unsigned long long)ph->p_filesz,

        (unsigned long long)ph->p_memsz
    );

}

const elf64_program_header_t *elf_program_header(const elf64_header_t *header, uint16_t index)
{
    if (header == NULL)
        return NULL;

    if (index >= header->e_phnum)
        return NULL;

    return (const elf64_program_header_t *)((const uint8_t *)header + header->e_phoff + index * header->e_phentsize);
}