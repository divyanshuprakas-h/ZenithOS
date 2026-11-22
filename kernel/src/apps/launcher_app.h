#ifndef LAUNCHER_APP_H
#define LAUNCHER_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void launcher_app_init(void);
void launcher_app_draw(Window *window);
Window *launcher_app_get_window(void);
void launcher_app_toggle(void);
bool launcher_app_is_focused(void);

#endif
