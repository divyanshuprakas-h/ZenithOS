#pragma once

#include "elf_types.h"

#include <stdint.h>

int elf_validate(const elf64_header_t *header);

void elf_print_header(const elf64_header_t *header);

const elf64_program_header_t *elf_program_header_t(const elf64_header_t *header, uint16_t index);

void elf_print_program_header(const elf64_program_header_t *ph);

const elf64_program_header_t *elf_program_header(const elf64_header_t *header, uint16_t index);



