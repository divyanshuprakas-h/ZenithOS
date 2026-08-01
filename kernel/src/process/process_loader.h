#pragma once

#include "process.h"
#include "../elf/elf.h"

int process_map_segment(
    process_t *process,
    const elf64_program_header_t *ph,
    const void *elf_base
);