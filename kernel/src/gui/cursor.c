#include "cursor.h"

#include "../drivers/mouse.h"
#include "../framebuffer/framebuffer.h"

void cursor_draw(int x, int y)
{
    for (int i = -5; i < 5; i++)
    {
        framebuffer_putpixel(x, y+i, 0xFFFFFF);
    }

    for (int i = -5; i < 5; i++)
    {
        framebuffer_putpixel(x+i, y, 0xFFFFFF);
    }
}