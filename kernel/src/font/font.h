#ifndef FONT_H
#define FONT_H

#include <stddef.h>
#include <stdint.h>

void draw_char(
    int x,
    int y,
    char C,
    uint32_t color
);

void draw_string(
    int x,
    int y,
    const char *str,
    uint32_t color
);

void draw_char_scaled(
    int x,
    int y,
    char c,
    uint32_t color,
    uint32_t scale
);

void draw_text_scaled(
    int x,
    int y,
    const char *text,
    uint32_t color,
    uint32_t scale,
    uint32_t letter_spacing
);

void font_set_scale(uint32_t scale);

uint32_t font_get_scale(void);

size_t font_text_width_scaled(
    const char *text,
    uint32_t scale,
    uint32_t letter_spacing
);

size_t font_text_width(const char *text);

#endif