#include "desktop.h"
#include "theme.h"
#include "gui_state.h"
#include "cursor.h"
#include "windows.h"
#include "window_manager.h"

#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../framebuffer/framebuffer.h"
#include "../drivers/mouse.h"
#include "../apps/launcher_app.h"
#include "../apps/terminal_app.h"
#include "../apps/files_app.h"
#include "../apps/sysmon_app.h"
#include "../apps/settings_app.h"
#include "../apps/calc_app.h"
#include "../mm/pmm.h"
#include "../hal/pit.h"
#include "../stdio/printf.h"
#include "../lib/string.h"

static void format_uint_local(uint64_t val, char *buf)
{
    char tmp[32];
    int i = 0;
    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0)
    {
        tmp[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    int j = 0;
    while (i > 0)
    {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void desktop_draw_stats_bar(void)
{
    size_t fb_w = framebuffer_width();
    if (fb_w == 0) return;

    int area_w = 260;
    int area_x = (int)fb_w - area_w;
    fill_rect(area_x, 1, area_w, TOPBAR_HEIGHT - 2, GUI_TOPBAR_BG);

    uint64_t total_mb = (pmm_get_total_pages() * 4096) / (1024 * 1024);
    uint64_t used_mb = (pmm_get_used_pages() * 4096) / (1024 * 1024);
    uint64_t sec = pit_ticks() / 100;
    uint64_t mins = sec / 60;
    uint64_t secs = sec % 60;

    char num_buf[32];
    char stats_buf[128];

    k_strcpy(stats_buf, "RAM: ");
    format_uint_local(used_mb, num_buf);
    int p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, "/");
    format_uint_local(total_mb, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, "MB  |  UP: ");
    format_uint_local(mins, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, "m ");
    format_uint_local(secs, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, num_buf);
    p = k_strlen(stats_buf);
    k_strcpy(stats_buf + p, "s");

    size_t stats_w = font_text_width_scaled(stats_buf, 1, 1);
    draw_text_scaled((int)fb_w - (int)stats_w - 18, 14, stats_buf, g_gui_accent, 1, 1);
}

static void desktop_draw_background(void)
{
    size_t fb_w = framebuffer_width();
    size_t fb_h = framebuffer_height();

    // 1. Dynamic smooth gradient wallpaper
    size_t grad_height = fb_h - TOPBAR_HEIGHT;
    uint8_t top_r = (uint8_t)((g_gui_bg_top >> 16) & 0xFF);
    uint8_t top_g = (uint8_t)((g_gui_bg_top >> 8) & 0xFF);
    uint8_t top_b = (uint8_t)(g_gui_bg_top & 0xFF);

    uint8_t bot_r = (uint8_t)((g_gui_bg_bottom >> 16) & 0xFF);
    uint8_t bot_g = (uint8_t)((g_gui_bg_bottom >> 8) & 0xFF);
    uint8_t bot_b = (uint8_t)(g_gui_bg_bottom & 0xFF);

    for (size_t y = TOPBAR_HEIGHT; y < fb_h; y++)
    {
        size_t rel_y = y - TOPBAR_HEIGHT;
        uint8_t r = top_r + (uint8_t)(((int)bot_r - (int)top_r) * (int)rel_y / (int)grad_height);
        uint8_t g = top_g + (uint8_t)(((int)bot_g - (int)top_g) * (int)rel_y / (int)grad_height);
        uint8_t b = top_b + (uint8_t)(((int)bot_b - (int)top_b) * (int)rel_y / (int)grad_height);
        uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;

        fill_rect(DOCK_WIDTH, y, fb_w - DOCK_WIDTH, 1, color);
    }

    // 2. Top Bar
    fill_rect(0, 0, fb_w, TOPBAR_HEIGHT, GUI_TOPBAR_BG);
    fill_rect(0, TOPBAR_HEIGHT - 1, fb_w, 1, GUI_TOPBAR_BORDER);

    // Brand logo
    draw_text_scaled(16, 10, "ZENITH", g_gui_accent, 2, 1);
    size_t z_w = font_text_width_scaled("ZENITH", 2, 1);
    draw_text_scaled(16 + z_w + 4, 10, "OS", GUI_TEXT_PRIMARY, 2, 1);
    size_t os_w = font_text_width_scaled("OS", 2, 1);
    draw_text_scaled(16 + z_w + 4 + os_w + 10, 14, "v0.1-alpha", GUI_TEXT_MUTED, 1, 1);

    // Center: Active window title
    Window *focused_win = window_manager_get_focused();
    if (focused_win != NULL && focused_win->title[0] != '\0')
    {
        size_t title_w = font_text_width_scaled(focused_win->title, 1, 1);
        int center_x = (int)(fb_w / 2) - (int)(title_w / 2);
        if (center_x > (int)(16 + z_w + 4 + os_w + 100))
        {
            fill_rect(center_x - 12, 17, 5, 5, g_gui_accent);
            draw_text_scaled(center_x, 14, focused_win->title, GUI_TEXT_SECONDARY, 1, 1);
        }
    }

    // Right side: Real OS Metrics
    desktop_draw_stats_bar();

    // 3. Dock
    fill_rect(0, TOPBAR_HEIGHT, DOCK_WIDTH, fb_h - TOPBAR_HEIGHT, GUI_DOCK_BG);
    fill_rect(DOCK_WIDTH - 1, TOPBAR_HEIGHT, 1, fb_h - TOPBAR_HEIGHT, GUI_DOCK_BORDER);

    /* --- Dock Item 0: Applications Launcher (APPS) --- */
    fill_rect(14, 50, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 50, 44, 44, GUI_DOCK_TILE_BORDER);

    // App launcher 2x2 grid icon
    fill_rect(24, 60, 10, 10, g_gui_accent);
    fill_rect(38, 60, 10, 10, g_gui_accent);
    fill_rect(24, 74, 10, 10, g_gui_accent);
    fill_rect(38, 74, 10, 10, g_gui_accent);

    draw_text_scaled(22, 98, "APPS", GUI_TEXT_SECONDARY, 1, 1);

    Window *apps_win = launcher_app_get_window();
    if (apps_win != NULL && apps_win->visible && !apps_win->minimised)
    {
        fill_rect(4, 64, 3, 16, g_gui_accent);
    }
    else if (apps_win != NULL && apps_win->minimised)
    {
        fill_rect(4, 69, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 1: Terminal (TERM) --- */
    fill_rect(14, 114, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 114, 44, 44, GUI_DOCK_TILE_BORDER);

    // Terminal prompt artwork `>_`
    draw_line(26, 130, 32, 136, 0x22C55E);
    draw_line(26, 131, 32, 137, 0x22C55E);
    draw_line(32, 136, 26, 142, 0x22C55E);
    draw_line(32, 137, 26, 143, 0x22C55E);
    draw_line(36, 142, 44, 142, 0x22C55E);
    draw_line(36, 143, 44, 143, 0x22C55E);

    draw_text_scaled(22, 162, "TERM", GUI_TEXT_SECONDARY, 1, 1);

    Window *term_win = terminal_app_get_window();
    if (term_win != NULL && term_win->visible && !term_win->minimised)
    {
        fill_rect(4, 128, 3, 16, g_gui_accent);
    }
    else if (term_win != NULL && term_win->minimised)
    {
        fill_rect(4, 133, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 2: Files (FILES) --- */
    fill_rect(14, 178, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 178, 44, 44, GUI_DOCK_TILE_BORDER);

    // Folder artwork
    fill_rect(24, 192, 10, 4, 0xD97706);
    fill_rect(24, 196, 24, 14, 0xF59E0B);
    fill_rect(26, 199, 20, 9, 0xFEF3C7);

    draw_text_scaled(18, 226, "FILES", GUI_TEXT_SECONDARY, 1, 1);

    Window *files_win = files_app_get_window();
    if (files_win != NULL && files_win->visible && !files_win->minimised)
    {
        fill_rect(4, 192, 3, 16, g_gui_accent);
    }
    else if (files_win != NULL && files_win->minimised)
    {
        fill_rect(4, 197, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 3: System Monitor (SYS) --- */
    fill_rect(14, 242, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 242, 44, 44, GUI_DOCK_TILE_BORDER);

    // Pulse wave graph artwork
    draw_line(22, 264, 27, 264, 0x38BDF8);
    draw_line(27, 264, 31, 254, 0x38BDF8);
    draw_line(31, 254, 35, 274, 0x38BDF8);
    draw_line(35, 274, 39, 264, 0x38BDF8);
    draw_line(39, 264, 48, 264, 0x38BDF8);

    draw_text_scaled(26, 290, "SYS", GUI_TEXT_SECONDARY, 1, 1);

    Window *sys_win = sysmon_app_get_window();
    if (sys_win != NULL && sys_win->visible && !sys_win->minimised)
    {
        fill_rect(4, 256, 3, 16, g_gui_accent);
    }
    else if (sys_win != NULL && sys_win->minimised)
    {
        fill_rect(4, 261, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 4: Settings (SET) --- */
    fill_rect(14, 306, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 306, 44, 44, GUI_DOCK_TILE_BORDER);

    // Sliders / Controls artwork
    draw_line(22, 322, 48, 322, 0xA855F7);
    fill_rect(28, 318, 6, 8, 0xA855F7);
    draw_line(22, 334, 48, 334, 0xA855F7);
    fill_rect(38, 330, 6, 8, 0xA855F7);

    draw_text_scaled(26, 354, "SET", GUI_TEXT_SECONDARY, 1, 1);

    Window *set_win = settings_app_get_window();
    if (set_win != NULL && set_win->visible && !set_win->minimised)
    {
        fill_rect(4, 320, 3, 16, g_gui_accent);
    }
    else if (set_win != NULL && set_win->minimised)
    {
        fill_rect(4, 325, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 5: Calculator (CALC) --- */
    fill_rect(14, 370, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 370, 44, 44, GUI_DOCK_TILE_BORDER);

    // Calculator artwork
    fill_rect(25, 378, 22, 27, 0x1A253A);
    draw_rect(25, 378, 22, 27, 0x2E3E5B);

    // Screen
    fill_rect(28, 381, 16, 6, 0x070B12);

    // Buttons
    fill_rect(28, 390, 6, 4, 0x38BDF8); // op
    fill_rect(37, 390, 6, 4, 0x38BDF8); // op
    fill_rect(28, 397, 6, 4, 0x94A3B8); // digit
    fill_rect(37, 397, 6, 4, 0x10B981); // equals

    draw_text_scaled(22, 418, "CALC", GUI_TEXT_SECONDARY, 1, 1);

    Window *calc_win = calc_app_get_window();
    if (calc_win != NULL && calc_win->visible && !calc_win->minimised)
    {
        fill_rect(4, 384, 3, 16, g_gui_accent);
    }
    else if (calc_win != NULL && calc_win->minimised)
    {
        fill_rect(4, 389, 3, 6, GUI_ACCENT_WARNING);
    }
}

void desktop_init(void)
{
    desktop_draw_background();
}

void desktop_draw(void)
{
    cursor_invalidate();

    desktop_draw_background();

    window_manager_draw();

    cursor_draw(mouse_state.x, mouse_state.y);
}

bool desktop_handle_mouse_down(int x, int y)
{
    if (x < DOCK_WIDTH)
    {
        // Item 0: APPS
        if (y >= 46 && y <= 104)
        {
            launcher_app_toggle();
            gui_needs_redraw = true;
            return true;
        }
        // Item 1: TERM
        else if (y >= 110 && y <= 168)
        {
            terminal_app_dock_click();
            gui_needs_redraw = true;
            return true;
        }
        // Item 2: FILES
        else if (y >= 174 && y <= 232)
        {
            files_app_dock_click();
            gui_needs_redraw = true;
            return true;
        }
        // Item 3: SYS
        else if (y >= 238 && y <= 296)
        {
            sysmon_app_dock_click();
            gui_needs_redraw = true;
            return true;
        }
        // Item 4: SET
        else if (y >= 302 && y <= 360)
        {
            settings_app_dock_click();
            gui_needs_redraw = true;
            return true;
        }
        // Item 5: CALC
        else if (y >= 366 && y <= 424)
        {
            calc_app_dock_click();
            gui_needs_redraw = true;
            return true;
        }

        return true; // Clicked on dock background, consume event
    }

    return false;
}
