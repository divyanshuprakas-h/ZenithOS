#ifndef GUI_STATE_H
#define GUI_STATE_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool gui_needs_redraw;

/* Dynamic system & desktop theme settings */
extern uint32_t g_gui_accent;
extern uint32_t g_gui_bg_top;
extern uint32_t g_gui_bg_bottom;
extern uint32_t g_compositor_interval;
extern uint32_t g_terminal_theme_color;
extern bool g_gui_active;

#endif
