#include "terminal_app.h"

#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../font/font.h"
#include "../terminal/terminal.h"

static void terminal_app_on_close(Window *window)
{
    (void)window;
    terminal_set_visible(false);
}

static Window terminal_window = {
    .x = 120,
    .y = 60,

    .width = 832,
    .height = 452,

    .saved_x = 120,
    .saved_y = 60,
    .saved_width = 832,
    .saved_height = 452,

    .visible = true,
    .focused = true,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Terminal - Ring 3 User Shell",

    .draw_content = terminal_app_draw,
    .on_close = terminal_app_on_close
};

void terminal_app_init(void)
{
    terminal_set_origin(terminal_window.x + 16, terminal_window.y + 36);
    terminal_set_visible(true);
    terminal_clear();
    window_manager_add(&terminal_window);
}

void terminal_app_draw(Window *window)
{
    terminal_set_origin(window->x + 16, window->y + 36);
    terminal_set_visible(true);
    terminal_render();
}

Window *terminal_app_get_window(void)
{
    return &terminal_window;
}

bool terminal_app_is_focused(void)
{
    return terminal_window.visible && !terminal_window.minimised && terminal_window.focused;
}

void terminal_app_dock_click(void)
{
    if (!terminal_window.visible || terminal_window.minimised)
    {
        terminal_window.visible = true;
        terminal_window.minimised = false;
        terminal_set_visible(true);
        window_manager_focus(&terminal_window);
    }
    else if (terminal_window.focused)
    {
        window_manager_minimize(&terminal_window);
    }
    else
    {
        window_manager_focus(&terminal_window);
    }
}
