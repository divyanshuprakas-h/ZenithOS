#include "elf.h"

#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../vmm/user_vm_layout.h"

int elf_validate(const elf64_header_t *header, size_t image_len)
{
    if (header == NULL)
        return KERNEL_EINVAL;

    if (image_len < sizeof(elf64_header_t))
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

    if (header->e_ident[6] != EV_CURRENT)
        return KERNEL_EINVAL;

    if (header->e_version != EV_CURRENT)
        return KERNEL_EINVAL;

    if (header->e_type != ET_EXEC)
        return KERNEL_EINVAL;

    if (header->e_machine != EM_X86_64)
        return KERNEL_EINVAL;

    if (header->e_ehsize < sizeof(elf64_header_t))
        return KERNEL_EINVAL;

    if (header->e_phentsize < sizeof(elf64_program_header_t))
        return KERNEL_EINVAL;

    if (header->e_phnum == 0 || header->e_phnum > 64)
        return KERNEL_EINVAL;

    // Check bounds of program header table against image length
    uint64_t ph_table_size = (uint64_t)header->e_phnum * header->e_phentsize;
    if (ph_table_size / header->e_phentsize != header->e_phnum)
        return KERNEL_EINVAL;

    if (header->e_phoff > image_len ||
        ph_table_size > image_len ||
        header->e_phoff + ph_table_size > image_len)
    {
        return KERNEL_EINVAL;
    }

    // Entry point must be in canonical user space
    if (header->e_entry < USER_BASE || header->e_entry >= USER_MAX_ADDRESS)
    {
        return KERNEL_EINVAL;
    }

    return KERNEL_SUCCESS;
}

void elf_print_header(const elf64_header_t *header)
{
    kprintf("========== ELF ==========\n");
    kprintf("Entry          : 0x%llx\n", (unsigned long long)header->e_entry);
    kprintf("PH Offset      : %llu\n", (unsigned long long)header->e_phoff);
    kprintf("PH Count       : %u\n", header->e_phnum);
    kprintf("SH Offset      : %llu\n", (unsigned long long)header->e_shoff);
    kprintf("SH Count       : %u\n", header->e_phnum);
    kprintf("=========================\n");
}

const elf64_program_header_t *elf_program_header(const elf64_header_t *header, uint16_t index, size_t image_len)
{
    if (header == NULL || index >= header->e_phnum)
        return NULL;

    uint64_t ph_offset = (uint64_t)index * header->e_phentsize;
    if (ph_offset / header->e_phentsize != index)
        return NULL;

    uint64_t offset = header->e_phoff + ph_offset;
    if (offset < header->e_phoff)
        return NULL;

    if (offset + sizeof(elf64_program_header_t) > image_len ||
        offset + sizeof(elf64_program_header_t) < offset)
    {
        return NULL;
    }

    return (const elf64_program_header_t *)((const uint8_t *)header + offset);
}

void elf_print_program_header(const elf64_program_header_t *ph)
{
    kprintf(
        "Type=%u Offset=%llu VAddr=0x%llx File=%llu Mem=%llu Flags=0x%x\n",
        ph->p_type,
        (unsigned long long)ph->p_offset,
        (unsigned long long)ph->p_vaddr,
        (unsigned long long)ph->p_filesz,
        (unsigned long long)ph->p_memsz,
        ph->p_flags
    );
}
