#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void settings_app_init(void);
void settings_app_draw(Window *window);
Window *settings_app_get_window(void);
void settings_app_dock_click(void);
bool settings_app_is_focused(void);

#endif
