#include "renderer.h"
#include "../framebuffer/framebuffer.h"

void renderer_clear(uint32_t color)
{
    framebuffer_clear(color);
}