#include "graphics.h"
#include "framebuffer/framebuffer.h"
#include "../font/font.h"
#include "../lib/math.h"

void draw_pixel(size_t x, size_t y, uint32_t color)
{
    size_t width = framebuffer_width();
    size_t height = framebuffer_height();

    if (x >= width || y >= height)
        return;
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
    size_t fb_w = framebuffer_width();
    size_t fb_h = framebuffer_height();

    if (x >= fb_w || y >= fb_h)
        return;

    if (x + width > fb_w)
        width = fb_w - x;

    if (y + height > fb_h)
        height = fb_h - y;

    uint32_t *fb = (uint32_t *)framebuffer_address();
    size_t pitch = framebuffer_pitch();

    for (size_t yy = y; yy < y + height; yy++)
    {
        uint32_t *row = fb + yy * pitch + x;
        for (size_t xx = 0; xx < width; xx++)
        {
            row[xx] = color;
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

void fill_rect(
    size_t x,
    size_t y,
    size_t width,
    size_t height,
    uint32_t color
)
{
    size_t fb_w = framebuffer_width();
    size_t fb_h = framebuffer_height();

    if (x >= fb_w || y >= fb_h)
        return;

    if (x + width > fb_w)
        width = fb_w - x;

    if (y + height > fb_h)
        height = fb_h - y;

    uint32_t *fb = (uint32_t *)framebuffer_address();
    size_t pitch = framebuffer_pitch();

    for (size_t py = y; py < y + height; py++)
    {
        uint32_t *row = fb + py * pitch + x;
        for (size_t px = 0; px < width; px++)
        {
            row[px] = color;
        }
    }
}

void draw_text(
    size_t x,
    size_t y,
    const char *text,
    uint32_t color
)
{
    if (!text)
        return;

    uint32_t scale = font_get_scale();

    while (*text)
    {
        if (*text == '\n')
        {
            x = 0;
            y += 8 * scale;
            text ++;
            continue;
        }
        draw_char_scaled(x, y, *text, color, scale);
        x += 8 * scale;
        text ++;
    }
}
