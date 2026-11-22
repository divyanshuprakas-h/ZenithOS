#pragma once

#include <stdbool.h>

void desktop_init(void);
void desktop_draw(void);
void desktop_draw_stats_bar(void);
bool desktop_handle_mouse_down(int x, int y);
