#include "framebuffer.h"
#include "../lib/memory.h"

static uint32_t *fb = 0;
static size_t fb_width =0;
static size_t fb_height =0;
static size_t fb_pitch =0;

static void *fb_address;

static uint32_t *back_buffer = 0;


void framebuffer_init(
    void *address,
    uint64_t width,
    uint64_t height,
    uint64_t pitch
)

{
    fb = (uint32_t *) address;
    fb_address = address;
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
    {
        return;
    }

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

void *framebuffer_address(void)
{
    return fb_address;
}

void framebuffer_scroll(size_t pixels, uint32_t clear_color)
{
    if (pixels >= fb_height)
    {
        framebuffer_clear(clear_color);
        return;
    }

    size_t row_size = fb_pitch * sizeof(uint32_t);
    k_memmove(
        fb,
        fb + pixels * fb_pitch,
        (fb_height - pixels) * row_size
    );

    uint32_t *bottom = fb + (fb_height - pixels) * fb_pitch;

    for (size_t y = 0; y < pixels; y++)
    {
        for (size_t x = 0; x < fb_width; x++)
        {
            bottom[y * fb_pitch + x] = clear_color;
        }
    }
    
}

size_t framebuffer_width(void)
{
    return fb_width;
}

size_t framebuffer_height(void)
{
    return fb_height;
}

size_t framebuffer_pitch(void)
{
    return fb_pitch;
}
