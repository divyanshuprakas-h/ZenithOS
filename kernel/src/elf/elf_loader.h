#pragma once

#include "elf.h"
#include "elf_types.h"

typedef struct 
{
    void *base;
    uint64_t size;
    uint64_t entry;
    uint64_t image_base;
} elf_image_t;


int elf_dump_segments(const elf64_header_t *header);

int elf_load_image(const void *image, elf_image_t *loaded);