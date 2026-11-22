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
    if (window == NULL || window_count >= MAX_WINDOWS)
    {
        return;
    }

    // Check if already in list
    for (int i = 0; i < window_count; i++)
    {
        if (windows[i] == window)
        {
            return;
        }
    }

    if (window->saved_width == 0 || window->saved_height == 0)
    {
        window->saved_x = window->x;
        window->saved_y = window->y;
        window->saved_width = window->width;
        window->saved_height = window->height;
    }

    windows[window_count++] = window;
    window_manager_focus(window);
}

void window_manager_draw(void)
{
    for (int i = 0; i < window_count; i++)
    {
        Window *w = windows[i];
        if (w != NULL && w->visible && !w->minimised)
        {
            window_draw(w);

            if (w->draw_content != NULL)
            {
                w->draw_content(w);
            }
        }
    }
}

Window *window_manager_get_window_at(int x, int y)
{
    for (int i = window_count - 1; i >= 0; i--)
    {
        Window *window = windows[i];
        if (window == NULL || !window->visible || window->minimised)
        {
            continue;
        }

        if (x >= window->x &&
            x < window->x + window->width &&
            y >= window->y &&
            y < window->y + window->height)
        {
            return window;
        }
    }
    return NULL;
}

Window *window_manager_get_focused(void)
{
    for (int i = window_count - 1; i >= 0; i--)
    {
        if (windows[i] != NULL && windows[i]->visible && !windows[i]->minimised)
        {
            if (windows[i]->focused)
            {
                return windows[i];
            }
        }
    }
    return NULL;
}

void window_manager_focus(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    for (int i = 0; i < window_count; i++)
    {
        if (windows[i] != NULL)
        {
            windows[i]->focused = (windows[i] == window);
        }
    }

    // Bring to top of Z-order
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

    window->visible = true;
    window->minimised = false;
    gui_needs_redraw = true;
}

void window_manager_minimize(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    window->minimised = true;
    window->visible = false;
    window->focused = false;

    if (drag_window == window)
    {
        drag_window = NULL;
    }

    // Focus the next topmost visible window
    for (int i = window_count - 1; i >= 0; i--)
    {
        if (windows[i] != NULL && windows[i]->visible && !windows[i]->minimised)
        {
            window_manager_focus(windows[i]);
            return;
        }
    }

    gui_needs_redraw = true;
}

void window_manager_maximize(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    if (!window->maximised)
    {
        window->saved_x = window->x;
        window->saved_y = window->y;
        window->saved_width = window->width;
        window->saved_height = window->height;

        window->x = DOCK_WIDTH + 2;
        window->y = TOPBAR_HEIGHT + 2;
        window->width = (int)framebuffer_width() - DOCK_WIDTH - 4;
        window->height = (int)framebuffer_height() - TOPBAR_HEIGHT - 4;
        window->maximised = true;
    }
    else
    {
        window->x = window->saved_x;
        window->y = window->saved_y;
        window->width = window->saved_width;
        window->height = window->saved_height;
        window->maximised = false;
    }

    gui_needs_redraw = true;
}

void window_manager_close(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    window->visible = false;
    window->focused = false;
    window->dragging = false;

    if (drag_window == window)
    {
        drag_window = NULL;
    }

    if (window->on_close != NULL)
    {
        window->on_close(window);
    }

    // Focus next topmost visible window
    for (int i = window_count - 1; i >= 0; i--)
    {
        if (windows[i] != NULL && windows[i]->visible && !windows[i]->minimised)
        {
            window_manager_focus(windows[i]);
            return;
        }
    }

    gui_needs_redraw = true;
}

bool window_manager_mouse_down(int x, int y)
{
    Window *window = window_manager_get_window_at(x, y);
    if (window == NULL)
    {
        return false;
    }

    // Focus window
    window_manager_focus(window);

    // Check control buttons first
    window_control_t control = window_hit_test_control(window, x, y);
    if (control == WINDOW_CONTROL_CLOSE)
    {
        window_manager_close(window);
        return true;
    }
    else if (control == WINDOW_CONTROL_MINIMIZE)
    {
        window_manager_minimize(window);
        return true;
    }
    else if (control == WINDOW_CONTROL_MAXIMIZE)
    {
        window_manager_maximize(window);
        return true;
    }

    // Check titlebar for dragging
    if (window_in_titlebar(window, x, y))
    {
        drag_window = window;
        window->dragging = true;
        window->drag_offset_x = x - window->x;
        window->drag_offset_y = y - window->y;
    }

    gui_needs_redraw = true;
    return true;
}

void window_manager_mouse_move(int x, int y)
{
    if (drag_window == NULL)
    {
        return;
    }

    if (drag_window->maximised)
    {
        // Unmaximize and center on cursor
        drag_window->maximised = false;
        drag_window->width = drag_window->saved_width;
        drag_window->height = drag_window->saved_height;
        drag_window->drag_offset_x = drag_window->width / 2;
        drag_window->drag_offset_y = TITLEBAR_HEIGHT / 2;
    }

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
    {
        return;
    }

    drag_window->dragging = false;
    drag_window = NULL;
    gui_needs_redraw = true;
}
