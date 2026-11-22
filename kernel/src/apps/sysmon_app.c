#include "sysmon_app.h"

#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../framebuffer/framebuffer.h"
#include "../mm/pmm.h"
#include "../hal/pit.h"
#include "../lib/string.h"

static void format_uint(uint64_t val, char *buf)
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

static void sysmon_app_on_close(Window *window)
{
    (void)window;
}

static Window sysmon_window = {
    .x = 220,
    .y = 80,

    .width = 540,
    .height = 370,

    .saved_x = 220,
    .saved_y = 80,
    .saved_width = 540,
    .saved_height = 370,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "System Monitor - ZenithOS",

    .draw_content = sysmon_app_draw,
    .on_close = sysmon_app_on_close
};

void sysmon_app_init(void)
{
    window_manager_add(&sysmon_window);
}

Window *sysmon_app_get_window(void)
{
    return &sysmon_window;
}

bool sysmon_app_is_focused(void)
{
    return sysmon_window.visible && !sysmon_window.minimised && sysmon_window.focused;
}

void sysmon_app_dock_click(void)
{
    if (!sysmon_window.visible || sysmon_window.minimised)
    {
        sysmon_window.visible = true;
        sysmon_window.minimised = false;
        window_manager_focus(&sysmon_window);
    }
    else if (sysmon_window.focused)
    {
        window_manager_minimize(&sysmon_window);
    }
    else
    {
        window_manager_focus(&sysmon_window);
    }
}

void sysmon_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return;
    }

    int x = window->x;
    int y = window->y;
    int w = window->width;
    int h = window->height;

    // Interior content background
    fill_rect(
        x + 2,
        y + TITLEBAR_HEIGHT,
        w - 4,
        h - TITLEBAR_HEIGHT - 2,
        0x0B0F19
    );

    int content_x = x + 24;
    int cursor_y = y + 46;

    /* ----------------------------------------------------
     * Section 1: Physical Memory (PMM)
     * ---------------------------------------------------- */
    draw_text_scaled(content_x, cursor_y, "PHYSICAL MEMORY", GUI_ACCENT_PRIMARY, 1, 1);
    cursor_y += 18;

    uint64_t total_pages = pmm_get_total_pages();
    uint64_t used_pages = pmm_get_used_pages();
    uint64_t free_pages = pmm_get_free_pages();

    uint64_t total_mb = (total_pages * 4096) / (1024 * 1024);
    uint64_t used_mb = (used_pages * 4096) / (1024 * 1024);
    uint64_t free_mb = (free_pages * 4096) / (1024 * 1024);

    uint64_t pct = (total_pages > 0) ? ((used_pages * 100) / total_pages) : 0;

    // Memory progress bar
    int bar_w = w - 48;
    int bar_h = 16;
    if (bar_w < 100) bar_w = 100;

    fill_rect(content_x, cursor_y, bar_w, bar_h, 0x141C2E);
    draw_rect(content_x, cursor_y, bar_w, bar_h, 0x2E3D52);

    int fill_w = (int)((bar_w - 4) * pct / 100);
    if (fill_w > 0)
    {
        fill_rect(content_x + 2, cursor_y + 2, fill_w, bar_h - 4, GUI_ACCENT_PRIMARY);
    }
    cursor_y += bar_h + 10;

    char num_buf[32];
    char text_buf[80];

    // Stats line
    k_strcpy(text_buf, "Used: ");
    format_uint(used_mb, num_buf);
    int p = k_strlen(text_buf);
    k_strcpy(text_buf + p, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, " MB  |  Free: ");
    format_uint(free_mb, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, " MB  |  Total: ");
    format_uint(total_mb, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, " MB (");
    format_uint(pct, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, num_buf);
    p = k_strlen(text_buf);
    k_strcpy(text_buf + p, "%)");

    draw_text_scaled(content_x, cursor_y, text_buf, GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 24;

    /* ----------------------------------------------------
     * Section 2: Hardware & Display
     * ---------------------------------------------------- */
    fill_rect(content_x, cursor_y, w - 48, 1, 0x1E293B);
    cursor_y += 10;

    draw_text_scaled(content_x, cursor_y, "HARDWARE & DISPLAY", GUI_ACCENT_PRIMARY, 1, 1);
    cursor_y += 18;

    draw_text_scaled(content_x, cursor_y, "Architecture  : x86_64 (64-bit AMD64 Long Mode)", GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 16;

    draw_text_scaled(content_x, cursor_y, "Boot Protocol : Limine Protocol (Higher-Half Direct Map)", GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 16;

    char disp_buf[64];
    k_strcpy(disp_buf, "Display Mode  : ");
    format_uint(framebuffer_width(), num_buf);
    p = k_strlen(disp_buf);
    k_strcpy(disp_buf + p, num_buf);
    p = k_strlen(disp_buf);
    k_strcpy(disp_buf + p, " x ");
    format_uint(framebuffer_height(), num_buf);
    p = k_strlen(disp_buf);
    k_strcpy(disp_buf + p, num_buf);
    p = k_strlen(disp_buf);
    k_strcpy(disp_buf + p, " @ 32 bpp (Linear Framebuffer)");
    draw_text_scaled(content_x, cursor_y, disp_buf, GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 24;

    /* ----------------------------------------------------
     * Section 3: Kernel Runtime & Scheduler
     * ---------------------------------------------------- */
    fill_rect(content_x, cursor_y, w - 48, 1, 0x1E293B);
    cursor_y += 10;

    draw_text_scaled(content_x, cursor_y, "KERNEL RUNTIME & MULTITASKING", GUI_ACCENT_PRIMARY, 1, 1);
    cursor_y += 18;

    uint64_t total_sec = pit_ticks() / 100;
    uint64_t hours = total_sec / 3600;
    uint64_t mins = (total_sec % 3600) / 60;
    uint64_t secs = total_sec % 60;

    char up_buf[64];
    k_strcpy(up_buf, "System Uptime : ");
    format_uint(hours, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, "h ");
    format_uint(mins, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, "m ");
    format_uint(secs, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, num_buf);
    p = k_strlen(up_buf);
    k_strcpy(up_buf + p, "s");
    draw_text_scaled(content_x, cursor_y, up_buf, GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 16;

    draw_text_scaled(content_x, cursor_y, "Timer Source  : 8254 PIT (100 Hz Preemptive Quantum)", GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 16;

    draw_text_scaled(content_x, cursor_y, "Tasks / Cores : Task 0 (Compositor), PID 1 (Ring 3 User Shell)", GUI_TEXT_SECONDARY, 1, 1);
    cursor_y += 16;

    draw_text_scaled(content_x, cursor_y, "Security Model: Ring 0 Kernel / Ring 3 User Separation", GUI_ACCENT_SUCCESS, 1, 1);
}
