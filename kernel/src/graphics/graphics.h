#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stddef.h>

void draw_pixel(size_t x, size_t y, uint32_t color);

void draw_rect(
    size_t x,
    size_t y,
    size_t width,
    size_t height,
    uint32_t color
);

void draw_line(
    int x0,
    int y0,
    int x1,
    int y1,
    uint32_t color
);

void fill_rect(
    size_t x,
    size_t y,
    size_t width,
    size_t height,
    uint32_t color
);

void draw_text(
    size_t x,
    size_t y,
    const char *text,
    uint32_t color
);

#endif