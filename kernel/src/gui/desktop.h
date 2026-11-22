#pragma once

#include <stdbool.h>

void desktop_init(void);
void desktop_draw(void);
bool desktop_handle_mouse_down(int x, int y);
