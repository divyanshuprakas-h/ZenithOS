#include "desktop.h"
#include "theme.h"
#include "cursor.h"
#include "windows.h"
#include "window_manager.h"

#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../framebuffer/framebuffer.h"
#include "../drivers/mouse.h"
#include "../apps/terminal_app.h"
#include "../apps/sysmon_app.h"
#include "../mm/pmm.h"
#include "../hal/pit.h"
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

static void desktop_draw_background(void)
{
    size_t fb_w = framebuffer_width();
    size_t fb_h = framebuffer_height();

    // 1. Smooth gradient wallpaper
    size_t grad_height = fb_h - TOPBAR_HEIGHT;
    for (size_t y = TOPBAR_HEIGHT; y < fb_h; y++)
    {
        size_t rel_y = y - TOPBAR_HEIGHT;
        uint8_t r = 0x0A + (uint8_t)((0x11 - 0x0A) * rel_y / grad_height);
        uint8_t g = 0x0E + (uint8_t)((0x18 - 0x0E) * rel_y / grad_height);
        uint8_t b = 0x17 + (uint8_t)((0x27 - 0x17) * rel_y / grad_height);
        uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;

        fill_rect(DOCK_WIDTH, y, fb_w - DOCK_WIDTH, 1, color);
    }

    // 2. Top Bar
    fill_rect(0, 0, fb_w, TOPBAR_HEIGHT, GUI_TOPBAR_BG);
    fill_rect(0, TOPBAR_HEIGHT - 1, fb_w, 1, GUI_TOPBAR_BORDER);

    // Brand logo
    draw_text_scaled(16, 10, "ZENITH", GUI_ACCENT_PRIMARY, 2, 1);
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
            fill_rect(center_x - 12, 17, 5, 5, GUI_ACCENT_PRIMARY);
            draw_text_scaled(center_x, 14, focused_win->title, GUI_TEXT_SECONDARY, 1, 1);
        }
    }

    // Right side: Real OS Metrics
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
    draw_text_scaled((int)fb_w - (int)stats_w - 18, 14, stats_buf, GUI_ACCENT_PRIMARY, 1, 1);

    // 3. Dock
    fill_rect(0, TOPBAR_HEIGHT, DOCK_WIDTH, fb_h - TOPBAR_HEIGHT, GUI_DOCK_BG);
    fill_rect(DOCK_WIDTH - 1, TOPBAR_HEIGHT, 1, fb_h - TOPBAR_HEIGHT, GUI_DOCK_BORDER);

    /* --- Dock Item 0: Terminal --- */
    fill_rect(14, 50, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 50, 44, 44, GUI_DOCK_TILE_BORDER);

    // Terminal prompt artwork `>_`
    draw_line(26, 66, 32, 72, 0x22C55E);
    draw_line(26, 67, 32, 73, 0x22C55E);
    draw_line(32, 72, 26, 78, 0x22C55E);
    draw_line(32, 73, 26, 79, 0x22C55E);
    draw_line(36, 78, 44, 78, 0x22C55E);
    draw_line(36, 79, 44, 79, 0x22C55E);

    draw_text_scaled(22, 98, "TERM", GUI_TEXT_SECONDARY, 1, 1);

    Window *term_win = terminal_app_get_window();
    if (term_win != NULL && term_win->visible && !term_win->minimised)
    {
        fill_rect(4, 64, 3, 16, GUI_ACCENT_PRIMARY);
    }
    else if (term_win != NULL && term_win->minimised)
    {
        fill_rect(4, 69, 3, 6, GUI_ACCENT_WARNING);
    }

    /* --- Dock Item 1: System Monitor --- */
    fill_rect(14, 114, 44, 44, GUI_DOCK_TILE);
    draw_rect(14, 114, 44, 44, GUI_DOCK_TILE_BORDER);

    // Pulse wave graph artwork
    draw_line(22, 136, 27, 136, GUI_ACCENT_PRIMARY);
    draw_line(27, 136, 31, 126, GUI_ACCENT_PRIMARY);
    draw_line(31, 126, 35, 146, GUI_ACCENT_PRIMARY);
    draw_line(35, 146, 39, 136, GUI_ACCENT_PRIMARY);
    draw_line(39, 136, 48, 136, GUI_ACCENT_PRIMARY);

    draw_text_scaled(25, 162, "SYS", GUI_TEXT_SECONDARY, 1, 1);

    Window *sys_win = sysmon_app_get_window();
    if (sys_win != NULL && sys_win->visible && !sys_win->minimised)
    {
        fill_rect(4, 128, 3, 16, GUI_ACCENT_PRIMARY);
    }
    else if (sys_win != NULL && sys_win->minimised)
    {
        fill_rect(4, 133, 3, 6, GUI_ACCENT_WARNING);
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
        // Dock hit
        if (y >= 46 && y <= 104)
        {
            terminal_app_dock_click();
            return true;
        }
        else if (y >= 110 && y <= 168)
        {
            sysmon_app_dock_click();
            return true;
        }

        return true; // Clicked on dock background, consume event
    }

    return false;
}
