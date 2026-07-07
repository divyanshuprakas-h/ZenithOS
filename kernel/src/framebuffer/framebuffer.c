#include "framebuffer.h"

static uint32_t *fb = 0;
static size_t fb_width =0;
static size_t fb_height =0;
static size_t fb_pitch =0;


void framebuffer_init(
    void *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch
)

{
    fb = (uint32_t *) address;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch/4;
}

void framebuffer_putpixel(
    size_t x,
    size_t y,
    uint32_t color
)

{
    if (x >= fb_width || y >= fb_height)
    return;

    fb[y * fb_pitch + x] = color;
}

void framebuffer_clear(uint32_t color)
{
    for (size_t y = 0; y < fb_height; y++)
    {
        for (size_t x = 0; x < fb_width; x++)
        {
            framebuffer_putpixel(x,y,color);
        }
    }
}
