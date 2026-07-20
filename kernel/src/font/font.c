#include "font.h"
#include "font8x8_data.h"

#include "../graphics/graphics.h"
#include "../lib/string.h"

static uint32_t font_scale = 2;
static uint32_t letter_spacing = 2;

void font_set_scale(uint32_t scale)
{
    if (scale == 0)
    {
        scale = 1;
    }

    font_scale = scale;
}

uint32_t font_get_scale(void)
{
    return font_scale;
}

void draw_char(
    int x,
    int y,
    char c,
    uint32_t color
)

{
    draw_char_scaled(
        x,
        y,
        c,
        color,
        font_scale
    );
}

void draw_string(
    int x,
    int y,
    const char *str,
    uint32_t color
)
{
    draw_text_scaled(
        x,
        y,
        str,
        color,
        font_scale,
        letter_spacing
    );
}

void draw_text_scaled(
    int x, 
    int y,
    const char *text,
    uint32_t color,
    uint32_t scale,
    uint32_t letter_spacing
)
{
    while (*text)
    {
        draw_char_scaled(
            x,
            y,
            *text,
            color,
            scale
        );

        x += 8 * scale + letter_spacing;
        text++;
    }
}

void draw_char_scaled(
    int x, 
    int y, 
    char c,
    uint32_t color,
    uint32_t scale
)
{
    if (c < 0 || c > 127)
        return;

    for (int row = 0; row < 8; row++)
    {
        uint8_t bits = font8x8_basic[(int)c][row];

        for (int col = 0; col < 8; col++)
        {
            if (bits & (1 << col))
            {
                draw_rect(
                    x + (col*scale),
                    y + (row*scale),
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

size_t font_text_width_scaled(const char *text, uint32_t scale, uint32_t letter_spacing)
{
    size_t len = k_strlen(text);

    if (len == 0)
        return 0;

    return (len * (8 * scale)) + ((len - 1) * letter_spacing);
}

size_t font_text_width(const char *text)
{
    return font_text_width_scaled(text, font_scale, 0);
}
