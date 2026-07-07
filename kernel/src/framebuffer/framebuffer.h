#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stddef.h>

void framebuffer_init(
    void *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch
);

void framebuffer_putpixel(
    size_t x,
    size_t y,
    uint32_t color
);

void framebuffer_clear(uint32_t color);

#endif