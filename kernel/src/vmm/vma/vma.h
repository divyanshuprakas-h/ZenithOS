#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    VMA_READ      = 1 << 0,
    VMA_WRITE     = 1 << 1,
    VMA_EXECUTE   = 1 << 2,

    VMA_DEMAND_PAGED = 1 << 3

}vma_flags_t;

typedef struct 
{
    uint64_t start;
    uint64_t end;

    uint64_t flags;
} vma_t;

void vma_init(void);

bool vma_create(
    uint64_t start,
    uint64_t size,
    uint64_t flags
);

const vma_t *vma_find(uint64_t address);

void vma_dump(void);

bool vma_destroy(uint64_t start);

bool vma_resize(uint64_t start, uint64_t new_size);