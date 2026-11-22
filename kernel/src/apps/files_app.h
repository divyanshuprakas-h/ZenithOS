#ifndef FILES_APP_H
#define FILES_APP_H

#include "../gui/windows.h"
#include <stdbool.h>

void files_app_init(void);
void files_app_draw(Window *window);
Window *files_app_get_window(void);
void files_app_dock_click(void);
bool files_app_is_focused(void);

#endif
