#include "terminal_app.h"

#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../font/font.h"
#include "../terminal/terminal.h"

static Window terminal_window = {
    .x = 120,
    .y = 70,

    .width = 832,
    .height = 452,

    .visible = true,
    .focused = true,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "ZenithOS Terminal - Ring 3",

    .draw_content = terminal_app_draw
};

void terminal_app_init(void)
{
    terminal_set_origin(terminal_window.x + 16, terminal_window.y + 36);
    terminal_clear();
    window_manager_add(&terminal_window);
}

void terminal_app_draw(Window *window)
{
    terminal_set_origin(window->x + 16, window->y + 36);
    terminal_render();
}
