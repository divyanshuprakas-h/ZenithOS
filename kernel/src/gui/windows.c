#include "windows.h"
#include "theme.h"

#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../framebuffer/framebuffer.h"

void window_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return;
    }

    int x = window->x;
    int y = window->y;
    int width = window->width;
    int height = window->height;

    // Soft drop shadow
    fill_rect(
        x + 4,
        y + 4,
        width,
        height,
        GUI_WIN_SHADOW
    );

    // Main window container background
    fill_rect(
        x,
        y,
        width,
        height,
        GUI_WIN_BG
    );

    // Titlebar
    uint32_t titlebar_bg = window->focused ? GUI_WIN_TITLEBAR_ACTIVE : GUI_WIN_TITLEBAR_INACTIVE;
    fill_rect(
        x,
        y,
        width,
        TITLEBAR_HEIGHT,
        titlebar_bg
    );

    // Titlebar bottom accent line
    if (window->focused)
    {
        fill_rect(
            x,
            y + TITLEBAR_HEIGHT - 2,
            width,
            2,
            GUI_ACCENT_PRIMARY
        );
    }
    else
    {
        fill_rect(
            x,
            y + TITLEBAR_HEIGHT - 1,
            width,
            1,
            GUI_WIN_BORDER_INACTIVE
        );
    }

    // Outer window border
    uint32_t border_color = window->focused ? GUI_WIN_BORDER_ACTIVE : GUI_WIN_BORDER_INACTIVE;
    fill_rect(x, y, width, 1, border_color);
    fill_rect(x, y + height - 1, width, 1, border_color);
    fill_rect(x, y, 1, height, border_color);
    fill_rect(x + width - 1, y, 1, height, border_color);

    // Window controls (Close, Minimize, Maximize)
    if (window->focused)
    {
        // Close (Red)
        fill_rect(x + 12, y + 10, 12, 12, GUI_ACCENT_DANGER);
        draw_rect(x + 12, y + 10, 12, 12, 0xB91C1C);
        draw_line(x + 15, y + 13, x + 20, y + 18, 0x7F1D1D);
        draw_line(x + 20, y + 13, x + 15, y + 18, 0x7F1D1D);

        // Minimize (Amber)
        fill_rect(x + 30, y + 10, 12, 12, GUI_ACCENT_WARNING);
        draw_rect(x + 30, y + 10, 12, 12, 0xB45309);
        draw_line(x + 33, y + 15, x + 38, y + 15, 0x78350F);

        // Maximize (Emerald)
        fill_rect(x + 48, y + 10, 12, 12, GUI_ACCENT_SUCCESS);
        draw_rect(x + 48, y + 10, 12, 12, 0x047857);
        draw_line(x + 51, y + 15, x + 56, y + 15, 0x064E3B);
        draw_line(x + 53, y + 13, x + 53, y + 18, 0x064E3B);
    }
    else
    {
        // Unfocused subdued buttons
        fill_rect(x + 12, y + 10, 12, 12, 0x2A374A);
        fill_rect(x + 30, y + 10, 12, 12, 0x2A374A);
        fill_rect(x + 48, y + 10, 12, 12, 0x2A374A);
    }

    // Titlebar text
    uint32_t text_color = window->focused ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY;
    draw_string(
        x + 72,
        y + 8,
        window->title,
        text_color
    );
}

bool window_in_titlebar(Window *window, int x, int y)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return false;
    }

    return x >= window->x &&
           x < window->x + window->width &&
           y >= window->y &&
           y < window->y + TITLEBAR_HEIGHT;
}

window_control_t window_hit_test_control(Window *window, int x, int y)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return WINDOW_CONTROL_NONE;
    }

    if (y < window->y + 6 || y > window->y + 26)
    {
        return WINDOW_CONTROL_NONE;
    }

    if (x >= window->x + 8 && x <= window->x + 25)
    {
        return WINDOW_CONTROL_CLOSE;
    }

    if (x >= window->x + 26 && x <= window->x + 43)
    {
        return WINDOW_CONTROL_MINIMIZE;
    }

    if (x >= window->x + 44 && x <= window->x + 62)
    {
        return WINDOW_CONTROL_MAXIMIZE;
    }

    return WINDOW_CONTROL_NONE;
}
