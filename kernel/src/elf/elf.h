#pragma once

#include "elf_types.h"
#include <stdint.h>
#include <stddef.h>

int elf_validate(const elf64_header_t *header, size_t image_len);

void elf_print_header(const elf64_header_t *header);

const elf64_program_header_t *elf_program_header(const elf64_header_t *header, uint16_t index, size_t image_len);

void elf_print_program_header(const elf64_program_header_t *ph);
