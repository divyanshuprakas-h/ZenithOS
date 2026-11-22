#include "device_studio_app.h"
#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../gui/gui_state.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../drivers/serial.h"
#include "../telemetry/telemetry.h"
#include "../hal/pit.h"
#include "../lib/string.h"
#include "../lib/memory.h"

#define STUDIO_WIN_WIDTH 640
#define STUDIO_WIN_HEIGHT 420

#define CONSOLE_MAX_LINES 64
#define CONSOLE_LINE_LEN 80
#define CONSOLE_VISIBLE_LINES 14

typedef enum {
    STUDIO_TAB_CONSOLE = 0,
    STUDIO_TAB_TELEMETRY = 1
} studio_tab_t;

static studio_tab_t s_active_tab = STUDIO_TAB_CONSOLE;

// Bounded console buffer
static char s_console_lines[CONSOLE_MAX_LINES][CONSOLE_LINE_LEN];
static uint32_t s_console_colors[CONSOLE_MAX_LINES];
static size_t s_console_count = 0;
static size_t s_console_head = 0; // index of next insert
static int s_scroll_offset = 0;   // 0 = bottom, >0 = scrolled up

// Input buffer
#define INPUT_MAX_LEN 64
static char s_input_buffer[INPUT_MAX_LEN];
static size_t s_input_len = 0;

// Streaming byte assembler
static char s_rx_line_buf[128];
static size_t s_rx_line_len = 0;

// Simulation timer
static uint64_t s_last_sim_tick = 0;
static bool s_sim_active = false;

static void device_studio_on_close(Window *window);
static bool device_studio_handle_mouse_down(Window *window, int x, int y);
static bool device_studio_handle_key(Window *window, char c);

static Window studio_window = {
    .x = 160,
    .y = 55,

    .width = STUDIO_WIN_WIDTH,
    .height = STUDIO_WIN_HEIGHT,

    .saved_x = 160,
    .saved_y = 55,
    .saved_width = STUDIO_WIN_WIDTH,
    .saved_height = STUDIO_WIN_HEIGHT,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Device Studio - Robotics & Serial Monitor",

    .draw_content = device_studio_app_draw,
    .on_close = device_studio_on_close,
    .handle_mouse_down = device_studio_handle_mouse_down,
    .handle_key = device_studio_handle_key
};

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

static void format_int(int64_t val, char *buf)
{
    if (val < 0)
    {
        buf[0] = '-';
        format_uint((uint64_t)(-val), buf + 1);
    }
    else
    {
        format_uint((uint64_t)val, buf);
    }
}

static void device_studio_append_console(const char *text, uint32_t color)
{
    if (text == NULL)
        return;

    size_t idx = s_console_head;
    k_strncpy(s_console_lines[idx], text, CONSOLE_LINE_LEN - 1);
    s_console_lines[idx][CONSOLE_LINE_LEN - 1] = '\0';
    s_console_colors[idx] = color;

    s_console_head = (s_console_head + 1) % CONSOLE_MAX_LINES;
    if (s_console_count < CONSOLE_MAX_LINES)
    {
        s_console_count++;
    }

    // Auto-scroll to bottom
    if (s_scroll_offset != 0)
    {
        s_scroll_offset = 0;
    }
}

static void device_studio_on_close(Window *window)
{
    (void)window;
}

void device_studio_app_init(void)
{
    telemetry_init();
    s_console_count = 0;
    s_console_head = 0;
    s_scroll_offset = 0;
    s_input_len = 0;
    s_input_buffer[0] = '\0';
    s_rx_line_len = 0;
    s_sim_active = false;
    s_active_tab = STUDIO_TAB_CONSOLE;

    device_studio_append_console("Zenith Device Studio v1.0 initialized.", 0x38BDF8);
    device_studio_append_console("Ready on /dev/ttyS0 (16550A UART, 115200 8N1).", 0x94A3B8);
    device_studio_append_console("Supported protocol: Zenith Telemetry #Z1.", 0x10B981);

    window_manager_add(&studio_window);
}

Window *device_studio_app_get_window(void)
{
    return &studio_window;
}

bool device_studio_app_is_focused(void)
{
    return studio_window.visible && !studio_window.minimised && studio_window.focused;
}

void device_studio_app_dock_click(void)
{
    if (!studio_window.visible || studio_window.minimised)
    {
        studio_window.visible = true;
        studio_window.minimised = false;
        window_manager_focus(&studio_window);
    }
    else if (studio_window.focused)
    {
        window_manager_minimize(&studio_window);
    }
    else
    {
        window_manager_focus(&studio_window);
    }
}

static void device_studio_send_input(void)
{
    if (s_input_len == 0)
        return;

    // Send to COM1 UART
    serial_write_bytes(s_input_buffer, s_input_len);
    serial_write_bytes("\r\n", 2);

    // Echo in console with prefix `> `
    char echo_buf[CONSOLE_LINE_LEN];
    echo_buf[0] = '>';
    echo_buf[1] = ' ';
    size_t i = 0;
    while (i < s_input_len && i < CONSOLE_LINE_LEN - 3)
    {
        echo_buf[2 + i] = s_input_buffer[i];
        i++;
    }
    echo_buf[2 + i] = '\0';
    device_studio_append_console(echo_buf, 0x22C55E);

    s_input_len = 0;
    s_input_buffer[0] = '\0';
    gui_needs_redraw = true;
}

void device_studio_poll(void)
{
    bool updated = false;

    // 1. Drain incoming serial data without blocking
    if (serial_is_present() && serial_available() > 0)
    {
        char temp[64];
        int n = serial_read_bytes(temp, sizeof(temp));
        for (int i = 0; i < n; i++)
        {
            char c = temp[i];
            if (c == '\r' || c == '\n')
            {
                if (s_rx_line_len > 0)
                {
                    s_rx_line_buf[s_rx_line_len] = '\0';

                    // Check if telemetry protocol line `#Z1,`
                    if (s_rx_line_len >= 4 && s_rx_line_buf[0] == '#' && s_rx_line_buf[1] == 'Z' && s_rx_line_buf[2] == '1')
                    {
                        if (telemetry_parse_line(s_rx_line_buf, s_rx_line_len))
                        {
                            device_studio_append_console(s_rx_line_buf, 0x06B6D4);
                        }
                        else
                        {
                            device_studio_append_console(s_rx_line_buf, 0xEF4444);
                        }
                    }
                    else
                    {
                        device_studio_append_console(s_rx_line_buf, 0xE2E8F0);
                    }

                    s_rx_line_len = 0;
                    updated = true;
                }
            }
            else if (s_rx_line_len < sizeof(s_rx_line_buf) - 1)
            {
                if (c >= 32 && c <= 126)
                {
                    s_rx_line_buf[s_rx_line_len++] = c;
                }
            }
        }
    }

    // 2. Periodic simulation tick if enabled
    if (s_sim_active)
    {
        uint64_t now = pit_ticks();
        if (now - s_last_sim_tick >= 50) // 2 Hz simulation stream (every 500ms)
        {
            s_last_sim_tick = now;
            telemetry_generate_sim_packet();

            const telemetry_record_t *rec = telemetry_get_current();
            if (rec != NULL)
            {
                char sim_buf[80];
                char num_buf[32];
                k_strcpy(sim_buf, "#Z1,");
                format_uint(rec->seq, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                format_int(rec->roll, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                format_int(rec->pitch, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                format_int(rec->yaw, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                format_uint(rec->voltage_mv, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                format_int(rec->temp_c, num_buf);
                k_strcat(sim_buf, num_buf);
                k_strcat(sim_buf, ",");
                k_strcat(sim_buf, rec->status);

                device_studio_append_console(sim_buf, 0xF59E0B);
                updated = true;
            }
        }
    }

    if (updated && studio_window.visible && !studio_window.minimised)
    {
        gui_needs_redraw = true;
    }
}

static bool device_studio_handle_key(Window *window, char c)
{
    (void)window;

    if (c == '\n' || c == '\r')
    {
        device_studio_send_input();
        return true;
    }
    else if (c == '\b' || c == 127) // Backspace
    {
        if (s_input_len > 0)
        {
            s_input_buffer[--s_input_len] = '\0';
            gui_needs_redraw = true;
        }
        return true;
    }
    else if (c >= 32 && c <= 126) // Printable char
    {
        if (s_input_len < INPUT_MAX_LEN - 1)
        {
            s_input_buffer[s_input_len++] = c;
            s_input_buffer[s_input_len] = '\0';
            gui_needs_redraw = true;
        }
        return true;
    }

    return false;
}

static bool device_studio_handle_mouse_down(Window *window, int x, int y)
{
    int win_x = window->x;
    int win_y = window->y;
    int rel_x = x - win_x;
    int rel_y = y - win_y;

    // Header bar controls (y: 28 .. 58)
    if (rel_y >= 28 && rel_y <= 58)
    {
        // Tab 0: Console button (x: 14 .. 94)
        if (rel_x >= 14 && rel_x <= 94)
        {
            s_active_tab = STUDIO_TAB_CONSOLE;
            gui_needs_redraw = true;
            return true;
        }
        // Tab 1: Telemetry button (x: 100 .. 190)
        if (rel_x >= 100 && rel_x <= 190)
        {
            s_active_tab = STUDIO_TAB_TELEMETRY;
            gui_needs_redraw = true;
            return true;
        }
        // Sim Toggle button (x: 440 .. 530)
        if (rel_x >= 440 && rel_x <= 530)
        {
            s_sim_active = !s_sim_active;
            telemetry_set_simulated(s_sim_active);
            if (s_sim_active)
            {
                device_studio_append_console("[SIMULATION] Telemetry stream started.", 0xF59E0B);
            }
            else
            {
                device_studio_append_console("[SIMULATION] Telemetry stream stopped.", 0x94A3B8);
            }
            gui_needs_redraw = true;
            return true;
        }
        // Clear button (x: 540 .. 620)
        if (rel_x >= 540 && rel_x <= 620)
        {
            s_console_count = 0;
            s_console_head = 0;
            s_scroll_offset = 0;
            device_studio_append_console("Console buffer cleared.", 0x64748B);
            gui_needs_redraw = true;
            return true;
        }
    }

    if (s_active_tab == STUDIO_TAB_CONSOLE)
    {
        // Scroll Up button (x: 614..630, y: 84..104)
        if (rel_x >= 614 && rel_x <= 630 && rel_y >= 84 && rel_y <= 104)
        {
            if (s_console_count > CONSOLE_VISIBLE_LINES && s_scroll_offset < (int)(s_console_count - CONSOLE_VISIBLE_LINES))
            {
                s_scroll_offset++;
                gui_needs_redraw = true;
            }
            return true;
        }
        // Scroll Down button (x: 614..630, y: 340..360)
        if (rel_x >= 614 && rel_x <= 630 && rel_y >= 340 && rel_y <= 360)
        {
            if (s_scroll_offset > 0)
            {
                s_scroll_offset--;
                gui_needs_redraw = true;
            }
            return true;
        }

        // Send Button (x: 550 .. 626, y: 374 .. 406)
        if (rel_x >= 550 && rel_x <= 626 && rel_y >= 374 && rel_y <= 406)
        {
            device_studio_send_input();
            return true;
        }
    }

    return false;
}

static void draw_meter_bar(int x, int y, int w, int h, int val, int min_v, int max_v, uint32_t color)
{
    fill_rect(x, y, w, h, 0x0B1220);
    draw_rect(x, y, w, h, 0x243044);

    if (min_v < 0 && max_v > 0)
    {
        // Center-zero bar (e.g. roll / pitch)
        int mid_x = x + w / 2;
        draw_line(mid_x, y, mid_x, y + h - 1, 0x475569);

        if (val > max_v) val = max_v;
        if (val < min_v) val = min_v;

        if (val > 0)
        {
            int fill_w = (val * (w / 2)) / max_v;
            if (fill_w > 0)
                fill_rect(mid_x + 1, y + 2, fill_w, h - 4, color);
        }
        else if (val < 0)
        {
            int fill_w = ((-val) * (w / 2)) / (-min_v);
            if (fill_w > 0)
                fill_rect(mid_x - fill_w, y + 2, fill_w, h - 4, color);
        }
    }
    else
    {
        // 0 to max_v bar (e.g. voltage)
        if (val < min_v) val = min_v;
        if (val > max_v) val = max_v;
        int span = max_v - min_v;
        if (span <= 0) span = 1;
        int fill_w = ((val - min_v) * (w - 4)) / span;
        if (fill_w > 0)
            fill_rect(x + 2, y + 2, fill_w, h - 4, color);
    }
}

void device_studio_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
        return;

    int x = window->x;
    int y = window->y;
    int w = window->width;
    int h = window->height;

    // Window Content Area
    int content_y = y + TITLEBAR_HEIGHT;
    int content_h = h - TITLEBAR_HEIGHT - 2;
    fill_rect(x + 2, content_y, w - 4, content_h, 0x0F1420);

    // --- Subheader / Tabs Bar (y + 28, height 32) ---
    int tab_y = y + 28;
    fill_rect(x + 4, tab_y, w - 8, 32, 0x0A0F1D);
    fill_rect(x + 4, tab_y + 31, w - 8, 1, 0x1E293B);

    // Tab 0: Console
    bool con_active = (s_active_tab == STUDIO_TAB_CONSOLE);
    fill_rect(x + 14, tab_y + 4, 80, 24, con_active ? 0x1E293B : 0x0F172A);
    draw_rect(x + 14, tab_y + 4, 80, 24, con_active ? 0x38BDF8 : 0x334155);
    draw_text_scaled(x + 24, tab_y + 10, "Console", con_active ? 0x38BDF8 : 0x94A3B8, 1, 1);

    // Tab 1: Telemetry
    bool tel_active = (s_active_tab == STUDIO_TAB_TELEMETRY);
    fill_rect(x + 100, tab_y + 4, 90, 24, tel_active ? 0x1E293B : 0x0F172A);
    draw_rect(x + 100, tab_y + 4, 90, 24, tel_active ? 0x38BDF8 : 0x334155);
    draw_text_scaled(x + 108, tab_y + 10, "Telemetry", tel_active ? 0x38BDF8 : 0x94A3B8, 1, 1);

    // Serial Hardware Status Badge
    serial_status_t ser;
    ser.is_online = serial_is_present();
    serial_get_status(&ser);

    int badge_x = x + 200;
    draw_text_scaled(badge_x, tab_y + 10, "COM1 115200 8N1", 0x64748B, 1, 1);
    badge_x += 128;
    if (ser.is_online)
    {
        fill_rect(badge_x, tab_y + 6, 68, 20, 0x064E3B);
        draw_rect(badge_x, tab_y + 6, 68, 20, 0x059669);
        draw_text_scaled(badge_x + 8, tab_y + 10, "ONLINE", 0x34D399, 1, 1);
    }
    else
    {
        fill_rect(badge_x, tab_y + 6, 68, 20, 0x451A03);
        draw_rect(badge_x, tab_y + 6, 68, 20, 0xD97706);
        draw_text_scaled(badge_x + 8, tab_y + 10, "OFFLINE", 0xFBBF24, 1, 1);
    }

    // Sim Button
    fill_rect(x + 440, tab_y + 4, 90, 24, s_sim_active ? 0x78350F : 0x1E293B);
    draw_rect(x + 440, tab_y + 4, 90, 24, s_sim_active ? 0xF59E0B : 0x475569);
    draw_text_scaled(x + 448, tab_y + 10, s_sim_active ? "Sim: ON" : "Sim: OFF", s_sim_active ? 0xFDE68A : 0x94A3B8, 1, 1);

    // Clear Button
    fill_rect(x + 540, tab_y + 4, 80, 24, 0x1E293B);
    draw_rect(x + 540, tab_y + 4, 80, 24, 0x475569);
    draw_text_scaled(x + 562, tab_y + 10, "Clear", GUI_TEXT_PRIMARY, 1, 1);

    // --- Tab Contents ---
    if (s_active_tab == STUDIO_TAB_CONSOLE)
    {
        // Status strip
        char stats_buf[128];
        char num[32];
        k_strcpy(stats_buf, "RX: ");
        format_uint(ser.bytes_rx, num);
        k_strcat(stats_buf, num);
        k_strcat(stats_buf, " B  |  TX: ");
        format_uint(ser.bytes_tx, num);
        k_strcat(stats_buf, num);
        k_strcat(stats_buf, " B  |  Overruns: ");
        format_uint(ser.rx_overruns, num);
        k_strcat(stats_buf, num);
        k_strcat(stats_buf, "  |  Dev: /dev/ttyS0");

        draw_text_scaled(x + 16, y + 66, stats_buf, 0x64748B, 1, 1);

        // Terminal Console Pane
        int pane_x = x + 14;
        int pane_y = y + 84;
        int pane_w = w - 46; // leave room for scroll buttons
        int pane_h = 276;

        fill_rect(pane_x, pane_y, pane_w, pane_h, 0x06090F);
        draw_rect(pane_x, pane_y, pane_w, pane_h, 0x1E293B);

        // Scroll controls on right of pane
        int sc_x = pane_x + pane_w + 4;
        fill_rect(sc_x, pane_y, 16, 20, 0x1E293B);
        draw_rect(sc_x, pane_y, 16, 20, 0x334155);
        draw_text_scaled(sc_x + 5, pane_y + 4, "^", 0x38BDF8, 1, 1);

        fill_rect(sc_x, pane_y + pane_h - 20, 16, 20, 0x1E293B);
        draw_rect(sc_x, pane_y + pane_h - 20, 16, 20, 0x334155);
        draw_text_scaled(sc_x + 5, pane_y + pane_h - 16, "v", 0x38BDF8, 1, 1);

        // Draw visible lines
        int visible_lines = CONSOLE_VISIBLE_LINES;
        int total_stored = (int)s_console_count;
        int start_pos = total_stored - visible_lines - s_scroll_offset;
        if (start_pos < 0) start_pos = 0;

        for (int l = 0; l < visible_lines; l++)
        {
            int line_logical_idx = start_pos + l;
            if (line_logical_idx >= total_stored)
                break;

            int physical_idx;
            if (s_console_count < CONSOLE_MAX_LINES)
            {
                physical_idx = line_logical_idx;
            }
            else
            {
                physical_idx = (s_console_head + line_logical_idx) % CONSOLE_MAX_LINES;
            }

            int line_y = pane_y + 8 + l * 18;
            draw_text_scaled(pane_x + 8, line_y, s_console_lines[physical_idx], s_console_colors[physical_idx], 1, 1);
        }

        // Bottom Input bar
        int in_y = y + h - 44;
        fill_rect(pane_x, in_y, pane_w - 80, 30, 0x0B1220);
        draw_rect(pane_x, in_y, pane_w - 80, 30, 0x334155);

        draw_text_scaled(pane_x + 8, in_y + 8, "Send >", 0x38BDF8, 1, 1);

        char display_in[INPUT_MAX_LEN + 2];
        k_strcpy(display_in, s_input_buffer);
        size_t dlen = k_strlen(display_in);
        display_in[dlen] = '_';
        display_in[dlen + 1] = '\0';

        draw_text_scaled(pane_x + 64, in_y + 8, display_in, GUI_TEXT_PRIMARY, 1, 1);

        // Send button
        int send_x = pane_x + pane_w - 72;
        fill_rect(send_x, in_y, 72, 30, 0x065F46);
        draw_rect(send_x, in_y, 72, 30, 0x10B981);
        draw_text_scaled(send_x + 18, in_y + 8, "Send", 0xE2E8F0, 1, 1);
    }
    else // STUDIO_TAB_TELEMETRY
    {
        const telemetry_record_t *rec = telemetry_get_current();
        const telemetry_stats_t *stats = telemetry_get_stats();

        // Status strip
        char tel_stats[128];
        char num[32];
        k_strcpy(tel_stats, "Source: ");
        k_strcat(tel_stats, stats->is_simulated ? "SIMULATED" : (stats->valid_packets > 0 ? "LIVE COM1" : "STANDBY"));
        k_strcat(tel_stats, "  |  Valid: ");
        format_uint(stats->valid_packets, num);
        k_strcat(tel_stats, num);
        k_strcat(tel_stats, "  |  Lost: ");
        format_uint(stats->dropped_packets, num);
        k_strcat(tel_stats, num);
        k_strcat(tel_stats, "  |  Err: ");
        format_uint(stats->invalid_packets, num);
        k_strcat(tel_stats, num);
        k_strcat(tel_stats, "  |  Proto: #Z1");

        draw_text_scaled(x + 16, y + 66, tel_stats, 0x38BDF8, 1, 1);

        // 6 Sensor Cards (Grid: 2 rows x 3 columns)
        int card_w = (w - 48) / 3;
        int card_h = 76;
        int grid_x = x + 16;
        int row1_y = y + 84;
        int row2_y = row1_y + card_h + 8;

        // Card 1: ROLL
        fill_rect(grid_x, row1_y, card_w, card_h, 0x131D31);
        draw_rect(grid_x, row1_y, card_w, card_h, 0x24324D);
        draw_text_scaled(grid_x + 10, row1_y + 8, "IMU ROLL", 0x94A3B8, 1, 1);
        char roll_buf[32];
        format_int(rec->roll, roll_buf);
        k_strcat(roll_buf, " deg");
        draw_text_scaled(grid_x + 10, row1_y + 24, roll_buf, 0x38BDF8, 2, 1);
        draw_meter_bar(grid_x + 10, row1_y + 50, card_w - 20, 16, rec->roll, -90, 90, 0x38BDF8);

        // Card 2: PITCH
        int col2_x = grid_x + card_w + 8;
        fill_rect(col2_x, row1_y, card_w, card_h, 0x131D31);
        draw_rect(col2_x, row1_y, card_w, card_h, 0x24324D);
        draw_text_scaled(col2_x + 10, row1_y + 8, "IMU PITCH", 0x94A3B8, 1, 1);
        char pitch_buf[32];
        format_int(rec->pitch, pitch_buf);
        k_strcat(pitch_buf, " deg");
        draw_text_scaled(col2_x + 10, row1_y + 24, pitch_buf, 0x22C55E, 2, 1);
        draw_meter_bar(col2_x + 10, row1_y + 50, card_w - 20, 16, rec->pitch, -90, 90, 0x22C55E);

        // Card 3: YAW / HEADING
        int col3_x = col2_x + card_w + 8;
        fill_rect(col3_x, row1_y, card_w, card_h, 0x131D31);
        draw_rect(col3_x, row1_y, card_w, card_h, 0x24324D);
        draw_text_scaled(col3_x + 10, row1_y + 8, "HEADING / YAW", 0x94A3B8, 1, 1);
        char yaw_buf[32];
        format_int(rec->yaw, yaw_buf);
        k_strcat(yaw_buf, " deg");
        const char *cardinal = "N";
        if (rec->yaw >= 23 && rec->yaw < 68) cardinal = "NE";
        else if (rec->yaw >= 68 && rec->yaw < 113) cardinal = "E";
        else if (rec->yaw >= 113 && rec->yaw < 158) cardinal = "SE";
        else if (rec->yaw >= 158 && rec->yaw < 203) cardinal = "S";
        else if (rec->yaw >= 203 && rec->yaw < 248) cardinal = "SW";
        else if (rec->yaw >= 248 && rec->yaw < 293) cardinal = "W";
        else if (rec->yaw >= 293 && rec->yaw < 338) cardinal = "NW";
        k_strcat(yaw_buf, " (");
        k_strcat(yaw_buf, cardinal);
        k_strcat(yaw_buf, ")");
        draw_text_scaled(col3_x + 10, row1_y + 24, yaw_buf, 0xA855F7, 2, 1);
        draw_meter_bar(col3_x + 10, row1_y + 50, card_w - 20, 16, rec->yaw, 0, 360, 0xA855F7);

        // Card 4: BATTERY VOLTAGE
        fill_rect(grid_x, row2_y, card_w, card_h, 0x131D31);
        draw_rect(grid_x, row2_y, card_w, card_h, 0x24324D);
        draw_text_scaled(grid_x + 10, row2_y + 8, "BATTERY VOLTAGE", 0x94A3B8, 1, 1);
        char volt_buf[32];
        uint32_t v_int = rec->voltage_mv / 1000;
        uint32_t v_dec = (rec->voltage_mv % 1000) / 10;
        format_uint(v_int, volt_buf);
        k_strcat(volt_buf, ".");
        if (v_dec < 10) k_strcat(volt_buf, "0");
        char dec_buf[16];
        format_uint(v_dec, dec_buf);
        k_strcat(volt_buf, dec_buf);
        k_strcat(volt_buf, " V");

        uint32_t v_color = (rec->voltage_mv >= 11500) ? 0x10B981 : (rec->voltage_mv >= 10500 ? 0xF59E0B : 0xEF4444);
        draw_text_scaled(grid_x + 10, row2_y + 24, volt_buf, v_color, 2, 1);
        draw_meter_bar(grid_x + 10, row2_y + 50, card_w - 20, 16, (int)rec->voltage_mv, 9000, 13000, v_color);

        // Card 5: TEMPERATURE
        fill_rect(col2_x, row2_y, card_w, card_h, 0x131D31);
        draw_rect(col2_x, row2_y, card_w, card_h, 0x24324D);
        draw_text_scaled(col2_x + 10, row2_y + 8, "TEMPERATURE", 0x94A3B8, 1, 1);
        char temp_buf[32];
        format_int(rec->temp_c, temp_buf);
        k_strcat(temp_buf, " C");
        draw_text_scaled(col2_x + 10, row2_y + 24, temp_buf, 0xFB923C, 2, 1);
        draw_meter_bar(col2_x + 10, row2_y + 50, card_w - 20, 16, rec->temp_c, 0, 70, 0xFB923C);

        // Card 6: STATUS & SEQUENCE
        fill_rect(col3_x, row2_y, card_w, card_h, 0x131D31);
        draw_rect(col3_x, row2_y, card_w, card_h, 0x24324D);
        draw_text_scaled(col3_x + 10, row2_y + 8, "NODE STATE", 0x94A3B8, 1, 1);
        draw_text_scaled(col3_x + 10, row2_y + 24, rec->status[0] ? rec->status : "IDLE", 0x38BDF8, 2, 1);
        char seq_buf[32];
        k_strcpy(seq_buf, "Seq: #");
        format_uint(rec->seq, num);
        k_strcat(seq_buf, num);
        draw_text_scaled(col3_x + 10, row2_y + 52, seq_buf, 0x64748B, 1, 1);

        // Live Sparkline Graph Area
        int graph_x = grid_x;
        int graph_y = row2_y + card_h + 10;
        int graph_w = w - 32;
        int graph_h = 130;

        fill_rect(graph_x, graph_y, graph_w, graph_h, 0x06090F);
        draw_rect(graph_x, graph_y, graph_w, graph_h, 0x1E293B);

        // Title and Legend
        draw_text_scaled(graph_x + 8, graph_y + 6, "LIVE ATTITUDE & VOLTAGE HISTORY", 0x64748B, 1, 1);
        int leg_x = graph_x + graph_w - 200;
        fill_rect(leg_x, graph_y + 9, 8, 2, 0x38BDF8);
        draw_text_scaled(leg_x + 12, graph_y + 6, "Roll", 0x94A3B8, 1, 1);

        leg_x += 54;
        fill_rect(leg_x, graph_y + 9, 8, 2, 0x22C55E);
        draw_text_scaled(leg_x + 12, graph_y + 6, "Pitch", 0x94A3B8, 1, 1);

        leg_x += 60;
        fill_rect(leg_x, graph_y + 9, 8, 2, 0xF59E0B);
        draw_text_scaled(leg_x + 12, graph_y + 6, "Volt", 0x94A3B8, 1, 1);

        // Center zero line
        int zero_y = graph_y + 24 + (graph_h - 28) / 2;
        for (int gx = graph_x + 4; gx < graph_x + graph_w - 4; gx += 8)
        {
            draw_line(gx, zero_y, gx + 4, zero_y, 0x1E293B);
        }

        // Plot samples
        const telemetry_history_t *hist = telemetry_get_history();
        if (hist->sample_count > 1)
        {
            int step_x = (graph_w - 20) / TELEMETRY_HISTORY_LEN;
            if (step_x < 2) step_x = 2;

            int prev_r_x = 0, prev_r_y = 0;
            int prev_p_x = 0, prev_p_y = 0;
            int prev_v_x = 0, prev_v_y = 0;

            for (size_t s = 0; s < hist->sample_count; s++)
            {
                size_t s_idx;
                if (hist->sample_count < TELEMETRY_HISTORY_LEN)
                {
                    s_idx = s;
                }
                else
                {
                    s_idx = (hist->write_idx + s) % TELEMETRY_HISTORY_LEN;
                }

                int px = graph_x + 10 + (int)s * step_x;

                // Roll: -90..+90 mapped to zero_y +- 40
                int ry = zero_y - (hist->roll_samples[s_idx] * 40) / 90;
                if (ry < graph_y + 20) ry = graph_y + 20;
                if (ry > graph_y + graph_h - 4) ry = graph_y + graph_h - 4;

                // Pitch: -90..+90 mapped to zero_y +- 40
                int py = zero_y - (hist->pitch_samples[s_idx] * 40) / 90;
                if (py < graph_y + 20) py = graph_y + 20;
                if (py > graph_y + graph_h - 4) py = graph_y + graph_h - 4;

                // Volt: 10000..13000 mV mapped to graph bottom .. top
                int vy = (graph_y + graph_h - 6) - (int)(((hist->volt_samples[s_idx] - 9500) * (graph_h - 32)) / 3500);
                if (vy < graph_y + 20) vy = graph_y + 20;
                if (vy > graph_y + graph_h - 4) vy = graph_y + graph_h - 4;

                if (s > 0)
                {
                    draw_line(prev_r_x, prev_r_y, px, ry, 0x38BDF8);
                    draw_line(prev_p_x, prev_p_y, px, py, 0x22C55E);
                    draw_line(prev_v_x, prev_v_y, px, vy, 0xF59E0B);
                }

                prev_r_x = px; prev_r_y = ry;
                prev_p_x = px; prev_p_y = py;
                prev_v_x = px; prev_v_y = vy;
            }
        }
    }
}
