#include "settings_app.h"
#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../gui/gui_state.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../terminal/terminal.h"

static bool settings_app_handle_mouse_down(Window *window, int x, int y);
static void settings_app_on_close(Window *window);

static Window settings_window = {
    .x = 200,
    .y = 90,

    .width = 500,
    .height = 370,

    .saved_x = 200,
    .saved_y = 90,
    .saved_width = 500,
    .saved_height = 370,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Settings - ZenithOS",

    .draw_content = settings_app_draw,
    .on_close = settings_app_on_close,
    .handle_mouse_down = settings_app_handle_mouse_down
};

static void settings_app_on_close(Window *window)
{
    (void)window;
}

void settings_app_init(void)
{
    window_manager_add(&settings_window);
}

Window *settings_app_get_window(void)
{
    return &settings_window;
}

bool settings_app_is_focused(void)
{
    return settings_window.visible && !settings_window.minimised && settings_window.focused;
}

void settings_app_dock_click(void)
{
    if (!settings_window.visible || settings_window.minimised)
    {
        settings_window.visible = true;
        settings_window.minimised = false;
        window_manager_focus(&settings_window);
    }
    else if (settings_window.focused)
    {
        window_manager_minimize(&settings_window);
    }
    else
    {
        window_manager_focus(&settings_window);
    }
}

static bool settings_app_handle_mouse_down(Window *win, int x, int y)
{
    int wx = win->x;
    int wy = win->y;

    // 1. Accent Colors: wy + 62 to wy + 94
    int swatch_y = wy + 62;
    int swatch_size = 28;
    int swatch_gap = 14;
    uint32_t accents[5] = {0x38BDF8, 0x10B981, 0xF59E0B, 0xA855F7, 0xF43F5E};

    for (int i = 0; i < 5; i++)
    {
        int sx = wx + 20 + i * (swatch_size + swatch_gap);
        if (x >= sx && x <= sx + swatch_size && y >= swatch_y && y <= swatch_y + swatch_size)
        {
            g_gui_accent = accents[i];
            gui_needs_redraw = true;
            return true;
        }
    }

    // 2. Wallpaper Themes: wy + 138 to wy + 168
    int wall_y = wy + 138;
    int wall_w = 140;
    int wall_h = 30;
    int wall_gap = 12;

    uint32_t bg_tops[3] = {0x0A0E17, 0x070B19, 0x121417};
    uint32_t bg_bots[3] = {0x111827, 0x0D1B2A, 0x1E2229};

    for (int i = 0; i < 3; i++)
    {
        int bx = wx + 20 + i * (wall_w + wall_gap);
        if (x >= bx && x <= bx + wall_w && y >= wall_y && y <= wall_y + wall_h)
        {
            g_gui_bg_top = bg_tops[i];
            g_gui_bg_bottom = bg_bots[i];
            gui_needs_redraw = true;
            return true;
        }
    }

    // 3. Terminal Color Theme: wy + 214 to wy + 244
    int term_y = wy + 214;
    int term_w = 104;
    int term_h = 28;
    int term_gap = 10;
    uint32_t term_colors[4] = {0xFFFFFF, 0x22C55E, 0xF59E0B, 0x38BDF8};

    for (int i = 0; i < 4; i++)
    {
        int tx = wx + 20 + i * (term_w + term_gap);
        if (x >= tx && x <= tx + term_w && y >= term_y && y <= term_y + term_h)
        {
            g_terminal_theme_color = term_colors[i];
            terminal_set_color(term_colors[i]);
            gui_needs_redraw = true;
            return true;
        }
    }

    // 4. Compositor Refresh: wy + 290 to wy + 320
    int comp_y = wy + 290;
    int comp_w = 140;
    int comp_h = 28;
    int comp_gap = 12;
    uint32_t intervals[3] = {100, 50, 20};

    for (int i = 0; i < 3; i++)
    {
        int cx = wx + 20 + i * (comp_w + comp_gap);
        if (x >= cx && x <= cx + comp_w && y >= comp_y && y <= comp_y + comp_h)
        {
            g_compositor_interval = intervals[i];
            gui_needs_redraw = true;
            return true;
        }
    }

    return true;
}

void settings_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
        return;

    int wx = window->x;
    int wy = window->y;
    int ww = window->width;
    int wh = window->height;

    // Content background
    fill_rect(wx + 2, wy + TITLEBAR_HEIGHT, ww - 4, wh - TITLEBAR_HEIGHT - 2, 0x0B0F19);

    // Section 1: System Accent Color
    draw_text_scaled(wx + 20, wy + 44, "SYSTEM ACCENT COLOR", g_gui_accent, 1, 1);

    int swatch_y = wy + 62;
    int swatch_size = 28;
    int swatch_gap = 14;
    uint32_t accents[5] = {0x38BDF8, 0x10B981, 0xF59E0B, 0xA855F7, 0xF43F5E};
    const char *accent_names[5] = {"Cyan", "Emerald", "Amber", "Purple", "Rose"};

    for (int i = 0; i < 5; i++)
    {
        int sx = wx + 20 + i * (swatch_size + swatch_gap);
        bool selected = (g_gui_accent == accents[i]);

        fill_rect(sx, swatch_y, swatch_size, swatch_size, accents[i]);
        if (selected)
        {
            draw_rect(sx - 2, swatch_y - 2, swatch_size + 4, swatch_size + 4, 0xFFFFFF);
            fill_rect(sx + 10, swatch_y + 10, 8, 8, 0xFFFFFF);
        }
        else
        {
            draw_rect(sx, swatch_y, swatch_size, swatch_size, 0x222E42);
        }
        draw_text_scaled(sx, swatch_y + 32, accent_names[i], selected ? GUI_TEXT_PRIMARY : GUI_TEXT_MUTED, 1, 1);
    }

    // Section 2: Wallpaper Gradient Theme
    draw_text_scaled(wx + 20, wy + 120, "WALLPAPER GRADIENT THEME", g_gui_accent, 1, 1);

    int wall_y = wy + 138;
    int wall_w = 140;
    int wall_h = 30;
    int wall_gap = 12;

    uint32_t bg_tops[3] = {0x0A0E17, 0x070B19, 0x121417};
    const char *wall_names[3] = {"Obsidian", "Deep Midnight", "Slate Charcoal"};

    for (int i = 0; i < 3; i++)
    {
        int bx = wx + 20 + i * (wall_w + wall_gap);
        bool selected = (g_gui_bg_top == bg_tops[i]);

        fill_rect(bx, wall_y, wall_w, wall_h, selected ? 0x1E293B : 0x121824);
        draw_rect(bx, wall_y, wall_w, wall_h, selected ? g_gui_accent : 0x243248);

        // Dot indicator
        fill_rect(bx + 10, wall_y + 11, 8, 8, selected ? g_gui_accent : 0x475569);
        draw_text_scaled(bx + 26, wall_y + 9, wall_names[i], selected ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1, 1);
    }

    // Section 3: Terminal Font Color
    draw_text_scaled(wx + 20, wy + 196, "TERMINAL TEXT COLOR", g_gui_accent, 1, 1);

    int term_y = wy + 214;
    int term_w = 104;
    int term_h = 28;
    int term_gap = 10;
    uint32_t term_colors[4] = {0xFFFFFF, 0x22C55E, 0xF59E0B, 0x38BDF8};
    const char *term_names[4] = {"White", "Green", "Amber", "Cyan"};

    for (int i = 0; i < 4; i++)
    {
        int tx = wx + 20 + i * (term_w + term_gap);
        bool selected = (g_terminal_theme_color == term_colors[i]);

        fill_rect(tx, term_y, term_w, term_h, selected ? 0x1E293B : 0x121824);
        draw_rect(tx, term_y, term_w, term_h, selected ? g_gui_accent : 0x243248);

        fill_rect(tx + 8, term_y + 10, 8, 8, term_colors[i]);
        draw_text_scaled(tx + 22, term_y + 8, term_names[i], term_colors[i], 1, 1);
    }

    // Section 4: Compositor Refresh Rate
    draw_text_scaled(wx + 20, wy + 272, "COMPOSITOR REFRESH INTERVAL", g_gui_accent, 1, 1);

    int comp_y = wy + 290;
    int comp_w = 140;
    int comp_h = 28;
    int comp_gap = 12;
    uint32_t intervals[3] = {100, 50, 20};
    const char *comp_names[3] = {"1 Hz (Calm)", "2 Hz (Balanced)", "5 Hz (Smooth)"};

    for (int i = 0; i < 3; i++)
    {
        int cx = wx + 20 + i * (comp_w + comp_gap);
        bool selected = (g_compositor_interval == intervals[i]);

        fill_rect(cx, comp_y, comp_w, comp_h, selected ? 0x1E293B : 0x121824);
        draw_rect(cx, comp_y, comp_w, comp_h, selected ? g_gui_accent : 0x243248);

        fill_rect(cx + 10, comp_y + 10, 8, 8, selected ? g_gui_accent : 0x475569);
        draw_text_scaled(cx + 24, comp_y + 8, comp_names[i], selected ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1, 1);
    }
}
