#include "gui_state.h"

volatile bool gui_needs_redraw = true;

/* Defaults: Zenith Sky, Obsidian Dark, Balanced 2Hz, Classic White */
uint32_t g_gui_accent = 0x38BDF8;
uint32_t g_gui_bg_top = 0x0A0E17;
uint32_t g_gui_bg_bottom = 0x111827;
uint32_t g_compositor_interval = 50; // 50 ticks = 2 Hz refresh for clock
uint32_t g_terminal_theme_color = 0xFFFFFF;
bool g_gui_active = false;

static int g_current_theme_id = 0;

int gui_get_theme(void)
{
    return g_current_theme_id;
}

void gui_set_theme(int theme_id)
{
    g_current_theme_id = theme_id;
    switch (theme_id)
    {
        case 0: // Dark / Slate
            g_gui_accent = 0x38BDF8;
            g_gui_bg_top = 0x0A0E17;
            g_gui_bg_bottom = 0x111827;
            break;
        case 1: // Matrix Green
            g_gui_accent = 0x22C55E;
            g_gui_bg_top = 0x021A0C;
            g_gui_bg_bottom = 0x072A14;
            break;
        case 2: // Nord / Arctic
            g_gui_accent = 0x88C0D0;
            g_gui_bg_top = 0x1E2430;
            g_gui_bg_bottom = 0x2E3440;
            break;
        case 3: // Amber / Retro CRT
            g_gui_accent = 0xF59E0B;
            g_gui_bg_top = 0x1A1204;
            g_gui_bg_bottom = 0x281B05;
            break;
        case 4: // Cyberpunk / Neon
            g_gui_accent = 0xF43F5E;
            g_gui_bg_top = 0x14051D;
            g_gui_bg_bottom = 0x280B3B;
            break;
        default:
            return;
    }
    gui_needs_redraw = true;
}
