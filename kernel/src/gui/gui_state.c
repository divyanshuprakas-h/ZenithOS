#include "gui_state.h"

volatile bool gui_needs_redraw = true;

/* Defaults: Zenith Sky, Obsidian Dark, Balanced 2Hz, Classic White */
uint32_t g_gui_accent = 0x38BDF8;
uint32_t g_gui_bg_top = 0x0A0E17;
uint32_t g_gui_bg_bottom = 0x111827;
uint32_t g_compositor_interval = 50; // 50 ticks = 2 Hz refresh for clock
uint32_t g_terminal_theme_color = 0xFFFFFF;
