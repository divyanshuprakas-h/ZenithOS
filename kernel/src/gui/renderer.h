#pragma once

#include <stdint.h>

void renderer_clear(uint32_t color);

void renderer_fill_rect(
    int x,
    int y,
    int width,
    int height,
    uint32_t color
);

void renderer_draw_text(
    int x,
    int y,
    const char *text,
    uint32_t color
);
