#include "window_manager.h"
#include "gui_state.h"
#include "theme.h"
#include "../framebuffer/framebuffer.h"
#include <stddef.h>

static Window *windows[MAX_WINDOWS];
static int window_count = 0;
static Window *drag_window = NULL;

void window_manager_init(void)
{
    window_count = 0;
    drag_window = NULL;
}

void window_manager_add(Window *window)
{
    if (window_count >= MAX_WINDOWS)
        return;

    windows[window_count++] = window;
}

void window_manager_draw(void)
{
    for (int i = 0; i < window_count; i++)
    {
        if (windows[i]->visible)
        {
            window_draw(windows[i]);

            if (windows[i]->draw_content)
            {
                windows[i]->draw_content(windows[i]);
            }
        }
    }
}

Window *window_manager_get_window_at(int x, int y)
{
    for (int i = window_count - 1; i >= 0; i--)
    {
        Window *window = windows[i];

        if (!window->visible)
            continue;

        if (
            x >= window->x &&
            x < window->x + window->width &&
            y >= window->y &&
            y < window->y + window->height
        )
        {
            return window;
        }
    }
    return NULL;
}

void window_manager_mouse_down(int x, int y)
{
    Window *window = window_manager_get_window_at(x, y);
    if (window == NULL)
        return;

    // Bring clicked window to top
    for (int i = 0; i < window_count; i++)
    {
        if (windows[i] == window)
        {
            for (int j = i; j < window_count - 1; j++)
            {
                windows[j] = windows[j + 1];
            }
            windows[window_count - 1] = window;
            break;
        }
    }

    if (!window_in_titlebar(window, x, y))
    {
        gui_needs_redraw = true;
        return;
    }

    drag_window = window;
    window->dragging = true;
    window->drag_offset_x = x - window->x;
    window->drag_offset_y = y - window->y;
    gui_needs_redraw = true;
}

void window_manager_mouse_move(int x, int y)
{
    if (drag_window == NULL)
        return;

    int new_x = x - drag_window->drag_offset_x;
    int new_y = y - drag_window->drag_offset_y;

    if (new_x < DOCK_WIDTH)
    {
        new_x = DOCK_WIDTH;
    }
    if (new_y < TOPBAR_HEIGHT)
    {
        new_y = TOPBAR_HEIGHT;
    }
    if (new_x + drag_window->width > (int)framebuffer_width())
    {
        new_x = (int)framebuffer_width() - drag_window->width;
    }
    if (new_y + drag_window->height > (int)framebuffer_height())
    {
        new_y = (int)framebuffer_height() - drag_window->height;
    }

    if (new_x != drag_window->x || new_y != drag_window->y)
    {
        drag_window->x = new_x;
        drag_window->y = new_y;
        gui_needs_redraw = true;
    }
}

void window_manager_mouse_up(void)
{
    if (drag_window == NULL)
        return;

    drag_window->dragging = false;
    drag_window = NULL;
    gui_needs_redraw = true;
}
