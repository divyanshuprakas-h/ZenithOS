#include "windows.h"
#include "theme.h"

#include "../graphics/graphics.h"
#include "../font/font.h"

void window_draw(
    Window *window
)
{

    int x = window->x;
    int y = window->y;

    int width = window->width;
    int height = window->height;

    const char *title = window->title;

    fill_rect(
        x + 4,
        y + 4,
        width,
        height,
        0x111827
    );

    fill_rect(
        x,
        y,
        width,
        height,
        0x1E293B
    );

    fill_rect(
        x,
        y,
        width,
        32,
        0x334155
    );

    fill_rect(x, y, width, 1, 0x64748B);
    fill_rect(x, y + height - 1, width, 1, 0x64748B);
    fill_rect(x, y, 1, height, 0x64748B);
    fill_rect(x + width - 1, y, 1, height, 0x64748B);

    fill_rect(x + 10, y + 10, 10, 10, 0xEF4444);
    fill_rect(x + 26, y + 10, 10, 10, 0xFACC15);
    fill_rect(x + 42, y + 10, 10, 10, 0x22C55E);

    draw_string(
        x + 80,
        y + 12,
        title,
        0xFFFFFF
    );
}

#define TITLEBAR_HEIGHT 24

bool window_in_titlebar(Window *window, int x, int y)
{
    return x >= window->x &&
           x < window->x + window->width &&
           y >= window->y &&
           y < window->y + TITLEBAR_HEIGHT;
}

