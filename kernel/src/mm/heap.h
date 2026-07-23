#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HEAP_START_ADDRESS 0xFFFF900000000000ULL
#define HEAP_INITIAL_SIZE (4ULL * 1024)
#define HEAP_MAX_SIZE (64ULL * 1024 * 1024)
#define HEAP_GROW_SIZE 4096ULL

bool heap_expand(void);

void heap_init(void);

void *kmalloc(size_t size);

void *kcalloc(size_t count, size_t size);

void *krealloc(void *ptr, size_t size);

void kfree(void *ptr);

#endif