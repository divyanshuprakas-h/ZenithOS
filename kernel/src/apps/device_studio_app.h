#ifndef DEVICE_STUDIO_APP_H
#define DEVICE_STUDIO_APP_H

#include <stdbool.h>
#include "../gui/windows.h"

void device_studio_app_init(void);
Window *device_studio_app_get_window(void);
bool device_studio_app_is_focused(void);
void device_studio_app_dock_click(void);
void device_studio_app_draw(Window *window);
void device_studio_poll(void);

#endif
