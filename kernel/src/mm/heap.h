#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool heap_expand(void);

void heap_init(void);

void *kmalloc(size_t size);

void *kcalloc(size_t count, size_t size);

void *krealloc(void *ptr, size_t size);

void kfree(void *ptr);

#endif