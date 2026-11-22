#include "launcher_app.h"
#include "terminal_app.h"
#include "files_app.h"
#include "sysmon_app.h"
#include "settings_app.h"

#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../stdio/printf.h"

static bool launcher_app_handle_mouse_down(Window *window, int x, int y);

static Window launcher_window = {
    .x = 140,
    .y = 60,

    .width = 480,
    .height = 320,

    .saved_x = 140,
    .saved_y = 60,
    .saved_width = 480,
    .saved_height = 320,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Applications",

    .draw_content = launcher_app_draw,
    .on_close = NULL,
    .handle_mouse_down = launcher_app_handle_mouse_down
};

void launcher_app_init(void)
{
    window_manager_add(&launcher_window);
}

Window *launcher_app_get_window(void)
{
    return &launcher_window;
}

bool launcher_app_is_focused(void)
{
    return launcher_window.visible && !launcher_window.minimised && launcher_window.focused;
}

void launcher_app_toggle(void)
{
    if (!launcher_window.visible || launcher_window.minimised)
    {
        launcher_window.visible = true;
        launcher_window.minimised = false;
        window_manager_focus(&launcher_window);
    }
    else if (launcher_window.focused)
    {
        window_manager_minimize(&launcher_window);
    }
    else
    {
        window_manager_focus(&launcher_window);
    }
}

static void draw_app_card(int x, int y, int w, int h, const char *name, const char *desc, const char *status, uint32_t status_color, int app_type)
{
    fill_rect(x, y, w, h, 0x141C2E);
    draw_rect(x, y, w, h, 0x243044);

    // Icon tile 32x32
    fill_rect(x + 12, y + 14, 32, 32, 0x1A243B);
    draw_rect(x + 12, y + 14, 32, 32, 0x2E3E5B);

    if (app_type == 0) // Terminal
    {
        draw_line(x + 20, y + 26, x + 25, y + 30, 0x22C55E);
        draw_line(x + 25, y + 30, x + 20, y + 34, 0x22C55E);
        draw_line(x + 28, y + 34, x + 35, y + 34, 0x22C55E);
    }
    else if (app_type == 1) // Files
    {
        fill_rect(x + 18, y + 25, 20, 13, 0xF59E0B);
        fill_rect(x + 18, y + 22, 9, 3, 0xD97706);
    }
    else if (app_type == 2) // System Monitor
    {
        draw_line(x + 17, y + 30, x + 21, y + 30, 0x38BDF8);
        draw_line(x + 21, y + 30, x + 24, y + 23, 0x38BDF8);
        draw_line(x + 24, y + 23, x + 28, y + 37, 0x38BDF8);
        draw_line(x + 28, y + 37, x + 31, y + 30, 0x38BDF8);
        draw_line(x + 31, y + 30, x + 39, y + 30, 0x38BDF8);
    }
    else if (app_type == 3) // Settings
    {
        draw_rect(x + 20, y + 22, 16, 16, 0xA855F7);
        fill_rect(x + 25, y + 27, 6, 6, 0xA855F7);
    }

    draw_text_scaled(x + 52, y + 16, name, GUI_TEXT_PRIMARY, 1, 1);
    draw_text_scaled(x + 52, y + 32, desc, GUI_TEXT_MUTED, 1, 1);

    // Status pill
    fill_rect(x + 14, y + 54, w - 28, 18, 0x0F1626);
    draw_text_scaled(x + 22, y + 59, "Status: ", GUI_TEXT_MUTED, 1, 1);
    draw_text_scaled(x + 80, y + 59, status, status_color, 1, 1);
}

void launcher_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return;
    }

    int x = window->x;
    int y = window->y;
    int w = window->width;
    int h = window->height;

    // Content background
    fill_rect(x + 2, y + TITLEBAR_HEIGHT, w - 4, h - TITLEBAR_HEIGHT - 2, 0x0B0F19);

    draw_text_scaled(x + 20, y + 44, "APPLICATIONS", g_gui_accent, 1, 1);
    draw_text_scaled(x + 20, y + 60, "Click any app to launch or bring to focus:", GUI_TEXT_SECONDARY, 1, 1);

    int card_w = (w - 56) / 2;
    int card_h = 80;

    int col1_x = x + 20;
    int col2_x = x + 20 + card_w + 16;
    int row1_y = y + 80;
    int row2_y = y + 80 + card_h + 16;

    // 0: Terminal
    Window *tw = terminal_app_get_window();
    const char *t_status = "Ready";
    uint32_t t_color = GUI_TEXT_MUTED;
    if (tw != NULL && tw->visible && !tw->minimised) { t_status = "Running"; t_color = GUI_ACCENT_SUCCESS; }
    else if (tw != NULL && tw->minimised) { t_status = "Minimized"; t_color = GUI_ACCENT_WARNING; }
    draw_app_card(col1_x, row1_y, card_w, card_h, "Terminal", "Ring 3 User Shell", t_status, t_color, 0);

    // 1: Files
    Window *fw = files_app_get_window();
    const char *f_status = "Ready";
    uint32_t f_color = GUI_TEXT_MUTED;
    if (fw != NULL && fw->visible && !fw->minimised) { f_status = "Running"; f_color = GUI_ACCENT_SUCCESS; }
    else if (fw != NULL && fw->minimised) { f_status = "Minimized"; f_color = GUI_ACCENT_WARNING; }
    draw_app_card(col2_x, row1_y, card_w, card_h, "Files", "VFS File Manager", f_status, f_color, 1);

    // 2: System Monitor
    Window *sw = sysmon_app_get_window();
    const char *s_status = "Ready";
    uint32_t s_color = GUI_TEXT_MUTED;
    if (sw != NULL && sw->visible && !sw->minimised) { s_status = "Running"; s_color = GUI_ACCENT_SUCCESS; }
    else if (sw != NULL && sw->minimised) { s_status = "Minimized"; s_color = GUI_ACCENT_WARNING; }
    draw_app_card(col1_x, row2_y, card_w, card_h, "System Monitor", "RAM & System Stats", s_status, s_color, 2);

    // 3: Settings
    Window *gw = settings_app_get_window();
    const char *g_status = "Ready";
    uint32_t g_color = GUI_TEXT_MUTED;
    if (gw != NULL && gw->visible && !gw->minimised) { g_status = "Running"; g_color = GUI_ACCENT_SUCCESS; }
    else if (gw != NULL && gw->minimised) { g_status = "Minimized"; g_color = GUI_ACCENT_WARNING; }
    draw_app_card(col2_x, row2_y, card_w, card_h, "Settings", "Theme & Config", g_status, g_color, 3);
}

static void ensure_open_and_focus(Window *win)
{
    if (win == NULL)
        return;

    win->visible = true;
    win->minimised = false;
    window_manager_focus(win);
}

static bool launcher_app_handle_mouse_down(Window *window, int x, int y)
{
    int w = window->width;
    int card_w = (w - 56) / 2;
    int card_h = 80;

    int col1_x = window->x + 20;
    int col2_x = window->x + 20 + card_w + 16;
    int row1_y = window->y + 80;
    int row2_y = window->y + 80 + card_h + 16;

    // Card 0: Terminal
    if (x >= col1_x && x <= col1_x + card_w && y >= row1_y && y <= row1_y + card_h)
    {
        launcher_app_toggle();
        ensure_open_and_focus(terminal_app_get_window());
        return true;
    }

    // Card 1: Files
    if (x >= col2_x && x <= col2_x + card_w && y >= row1_y && y <= row1_y + card_h)
    {
        launcher_app_toggle();
        ensure_open_and_focus(files_app_get_window());
        return true;
    }

    // Card 2: System Monitor
    if (x >= col1_x && x <= col1_x + card_w && y >= row2_y && y <= row2_y + card_h)
    {
        launcher_app_toggle();
        ensure_open_and_focus(sysmon_app_get_window());
        return true;
    }

    // Card 3: Settings
    if (x >= col2_x && x <= col2_x + card_w && y >= row2_y && y <= row2_y + card_h)
    {
        launcher_app_toggle();
        ensure_open_and_focus(settings_app_get_window());
        return true;
    }

    return false;
}
