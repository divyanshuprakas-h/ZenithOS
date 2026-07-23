#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "windows.h"

#define MAX_WINDOWS 32

void window_manager_init(void);

void window_manager_add(Window *window);

void window_manager_draw(void);

Window *window_manager_get_window_at(int x, int y);

void window_manager_mouse_down(int x, int y);

void window_manager_mouse_move(int x, int y);

void window_manager_mouse_up(void);

#endif


