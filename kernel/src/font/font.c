#include "font.h"
#include "font8x8_data.h"

#include "../graphics/graphics.h"

void draw_char(
    int x,
    int y,
    char c,
    uint32_t color
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
                    x + (col*2),
                    y + (row*2),
                    2,
                    2,
                    color
                );
            }
        }
    }
}

void draw_string(
    int x,
    int y,
    const char *str,
    uint32_t color
)
{
    while (*str)
    {
        draw_char(
            x,
            y,
            *str,
            color
        );

        x += 16;
        str++;
    }
}
