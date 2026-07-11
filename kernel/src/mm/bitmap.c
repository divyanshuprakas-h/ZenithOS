#include "bitmap.h"

static uint8_t *bitmap;
static uint64_t bitmap_bits;

void bitmap_init(uint8_t *buffer, uint64_t bits)
{
    bitmap = buffer;
    bitmap_bits = bits;

    uint64_t bytes = (bits + 7) / 8;

    for (uint64_t i = 0; i < bytes; i++)
    {
        bitmap[i] = 0xFF;
    }
}

void bitmap_set(uint64_t bit)
{
    if (bit >= bitmap_bits)
        return;

    bitmap[bit / 8] |= (1u << (bit % 8));
}

void bitmap_clear(uint64_t bit)
{
    if (bit >= bitmap_bits)
        return;

    bitmap[bit / 8] &= ~(1u << (bit % 8));
}

bool bitmap_test(uint64_t bit)
{
    if (bit >= bitmap_bits)
        return true;

    return (bitmap[bit / 8] & (1u << (bit % 8))) != 0;
}
