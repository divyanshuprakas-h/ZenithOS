#ifndef SYSMON_APP_H
#define SYSMON_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void sysmon_app_init(void);
void sysmon_app_draw(Window *window);
Window *sysmon_app_get_window(void);
void sysmon_app_dock_click(void);
bool sysmon_app_is_focused(void);

#endif
