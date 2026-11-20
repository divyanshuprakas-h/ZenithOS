#include "cursor.h"
#include "../framebuffer/framebuffer.h"
#include <stdbool.h>

#define CURSOR_W 11
#define CURSOR_H 16

static const uint8_t cursor_shape[CURSOR_H][CURSOR_W] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0 },
    { 1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0 },
    { 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 2, 1, 0, 1, 2, 1, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 1, 2, 1, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
};

static uint32_t cursor_saved[CURSOR_H][CURSOR_W];
static int last_cursor_x = -1;
static int last_cursor_y = -1;
static bool cursor_visible = false;

void cursor_restore(void)
{
    if (!cursor_visible || last_cursor_x < 0 || last_cursor_y < 0)
    {
        return;
    }

    for (int r = 0; r < CURSOR_H; r++)
    {
        for (int c = 0; c < CURSOR_W; c++)
        {
            if (cursor_shape[r][c] != 0)
            {
                framebuffer_putpixel(last_cursor_x + c, last_cursor_y + r, cursor_saved[r][c]);
            }
        }
    }

    cursor_visible = false;
}

void cursor_draw(int x, int y)
{
    // Save background pixels under the cursor
    for (int r = 0; r < CURSOR_H; r++)
    {
        for (int c = 0; c < CURSOR_W; c++)
        {
            cursor_saved[r][c] = framebuffer_getpixel(x + c, y + r);
        }
    }

    last_cursor_x = x;
    last_cursor_y = y;
    cursor_visible = true;

    // Draw cursor
    for (int r = 0; r < CURSOR_H; r++)
    {
        for (int c = 0; c < CURSOR_W; c++)
        {
            uint8_t pixel = cursor_shape[r][c];
            if (pixel == 1)
            {
                framebuffer_putpixel(x + c, y + r, 0x000000);
            }
            else if (pixel == 2)
            {
                framebuffer_putpixel(x + c, y + r, 0xFFFFFF);
            }
        }
    }
}

void cursor_move(int x, int y)
{
    if (x == last_cursor_x && y == last_cursor_y && cursor_visible)
    {
        return;
    }

    cursor_restore();
    cursor_draw(x, y);
}

void cursor_invalidate(void)
{
    cursor_visible = false;
}
