#pragma once

#include <stdint.h>

#define EI_NIDENT   16

#define ELFCLASS64  2

#define ELFDATA2LSB 1

#define ET_EXEC     2

#define EM_X86_64   62

#define EV_CURRENT  1

typedef struct 
{
    unsigned char e_ident[EI_NIDENT];

    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;

    uint64_t e_entry;

    uint64_t e_phoff;
    uint64_t e_shoff;

    uint32_t e_flags;

    uint16_t e_ehsize;

    uint16_t e_phentsize;
    uint16_t e_phnum;

    uint16_t e_shentsize;
    uint16_t e_shnum;

    uint16_t e_shstrndx;

} elf64_header_t;

typedef struct 
{
    uint32_t p_type;
    uint32_t p_flags;

    uint64_t p_offset;

    uint64_t p_vaddr;

    uint64_t p_paddr;

    uint64_t p_filesz;

    uint64_t p_memsz;

    uint64_t p_align;

} elf64_program_header_t;


#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_TLS      7

#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4
