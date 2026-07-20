#include "terminal_app.h"

#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../font/font.h"
#include "../terminal/terminal.h"

static Window terminal_window = {
    .x = 150,
    .y = 80,

    .width = 900,
    .height = 600,

    .visible = true,
    .focused = true,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Terminal",

    .draw_content = terminal_app_draw
};

void terminal_app_init(void)
{
    terminal_init();

    terminal_write("ZenithOS Terminal v0.1\n\n");
    terminal_write("Type \"help\" to begin\n\n");

    terminal_set_color(0x22C55E);
    terminal_write("$");

    terminal_set_color(0xFFFFFF);

    window_manager_add(&terminal_window);

}

void terminal_app_draw(Window *window)
{

    terminal_set_origin(
        window->x + 20,
        window->y + 40
    );

    terminal_render();
}
