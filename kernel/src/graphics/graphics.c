#include "graphics.h"
#include "framebuffer/framebuffer.h"
#include "../lib/math.h"

void draw_pixel(size_t x, size_t y, uint32_t color)
{
    framebuffer_putpixel(x, y, color);
}

void draw_rect(
    size_t x,
    size_t y,
    size_t width,
    size_t height,
    uint32_t color
)

{
    for (size_t yy = y; yy < y+height ; yy++)
    {
        for (size_t xx = x; xx < x+height ; xx++)
        {
            draw_pixel(xx, yy, color);
        }
    }
}

void draw_line(
    int x0,
    int y0,
    int x1,
    int y1,
    uint32_t color
)

{
    int dx = abs_int(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -abs_int(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while(1)
    {
        draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy){
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx){
            err += dx;
            y0 += sy;
        }
    }
}