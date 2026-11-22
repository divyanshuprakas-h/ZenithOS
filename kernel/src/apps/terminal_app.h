#ifndef TERMINAL_APP_H
#define TERMINAL_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void terminal_app_init(void);
void terminal_app_draw(Window *window);
Window *terminal_app_get_window(void);
void terminal_app_dock_click(void);
bool terminal_app_is_focused(void);

#endif
