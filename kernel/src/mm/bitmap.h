#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <stdbool.h>

void bitmap_init(uint8_t *buffer, uint64_t bits);

void bitmap_set(uint64_t bit);

void bitmap_clear(uint64_t bit);

bool bitmap_test(uint64_t bit);

#endif
