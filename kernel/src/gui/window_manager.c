#include "window_manager.h" 
#include <stddef.h>

static Window *windows[MAX_WINDOWS];

static int window_count = 0;

static Window *drag_window = NULL;

void window_manager_init(void)
{
    window_count = 0;
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
    return "NULL";
}

void window_manager_mouse_down(int x, int y)
{
    Window *window = window_manager_get_window_at(x, y);

    if (window == NULL)
        return;

    if (!window_in_titlebar(window, x, y))
        return;

    drag_window = window;

    window->dragging = true;

    window->drag_offset_x = x - window->x;
    window->drag_offset_y = y - window->y;
}

void window_manager_mouse_move(int x, int y)
{
    if (drag_window == NULL)
        return;

    drag_window->x = x - drag_window->drag_offset_x;
    drag_window->y = y - drag_window->drag_offset_y;
}

void window_manager_mouse_up(void)
{
    if (drag_window == NULL)
        return;

    drag_window->dragging = false;
    drag_window = NULL;
}
