#ifndef CALC_APP_H
#define CALC_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void calc_app_init(void);
Window *calc_app_get_window(void);
void calc_app_draw(Window *window);
void calc_app_dock_click(void);
bool calc_app_is_focused(void);

#endif
