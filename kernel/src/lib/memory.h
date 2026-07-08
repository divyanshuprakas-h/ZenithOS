#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void *k_memcpy(void *restrict dest, const void *restrict src, size_t n);
void *k_memset(void *s, int c, size_t n);
void *k_memmove(void *dest, const void *src, size_t n);
int k_memcmp(const void *s1, const void *s2, size_t n);

#endif
