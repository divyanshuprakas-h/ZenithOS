#include "calc_app.h"
#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../gui/gui_state.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../lib/string.h"

typedef enum
{
    OP_NONE = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD
} calc_op_t;

static int64_t s_current = 0;
static int64_t s_stored = 0;
static calc_op_t s_op = OP_NONE;
static bool s_new_num = true;
static bool s_has_error = false;
static char s_error_text[24] = {0};
static char s_expr_text[48] = {0};

static bool calc_app_handle_mouse_down(Window *window, int x, int y);
static bool calc_app_handle_key(Window *window, char c);
static void calc_app_on_close(Window *window);

static Window calc_window = {
    .x = 260,
    .y = 110,

    .width = 252,
    .height = 334,

    .saved_x = 260,
    .saved_y = 110,
    .saved_width = 252,
    .saved_height = 334,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Calculator",

    .draw_content = calc_app_draw,
    .on_close = calc_app_on_close,
    .handle_mouse_down = calc_app_handle_mouse_down,
    .handle_key = calc_app_handle_key
};

static void calc_app_on_close(Window *window)
{
    (void)window;
}

void calc_app_init(void)
{
    window_manager_add(&calc_window);
}

Window *calc_app_get_window(void)
{
    return &calc_window;
}

bool calc_app_is_focused(void)
{
    return calc_window.visible && !calc_window.minimised && calc_window.focused;
}

void calc_app_dock_click(void)
{
    if (!calc_window.visible || calc_window.minimised)
    {
        calc_window.visible = true;
        calc_window.minimised = false;
        window_manager_focus(&calc_window);
    }
    else if (calc_window.focused)
    {
        window_manager_minimize(&calc_window);
    }
    else
    {
        window_manager_focus(&calc_window);
    }
}

static void format_int64(int64_t val, char *buf)
{
    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    int i = 0;
    bool neg = false;
    uint64_t uval;

    if (val < 0)
    {
        neg = true;
        uval = (uint64_t)(-(val + 1)) + 1ULL;
    }
    else
    {
        uval = (uint64_t)val;
    }

    char temp[32];
    int ti = 0;
    while (uval > 0)
    {
        temp[ti++] = (char)('0' + (uval % 10));
        uval /= 10;
    }

    if (neg)
    {
        buf[i++] = '-';
    }

    while (ti > 0)
    {
        buf[i++] = temp[--ti];
    }
    buf[i] = '\0';
}

static void format_hex64(int64_t val, char *buf)
{
    uint64_t u = (uint64_t)val;
    static const char hex_digits[] = "0123456789ABCDEF";

    buf[0] = 'H';
    buf[1] = 'E';
    buf[2] = 'X';
    buf[3] = ':';
    buf[4] = ' ';
    buf[5] = '0';
    buf[6] = 'x';

    if (u == 0)
    {
        buf[7] = '0';
        buf[8] = '\0';
        return;
    }

    int nibbles = 16;
    while (nibbles > 1 && ((u >> ((nibbles - 1) * 4)) & 0xF) == 0)
    {
        nibbles--;
    }

    int p = 7;
    for (int i = nibbles - 1; i >= 0; i--)
    {
        buf[p++] = hex_digits[(u >> (i * 4)) & 0xF];
    }
    buf[p] = '\0';
}

static void calc_clear_all(void)
{
    s_current = 0;
    s_stored = 0;
    s_op = OP_NONE;
    s_new_num = true;
    s_has_error = false;
    s_error_text[0] = '\0';
    s_expr_text[0] = '\0';
}

static void calc_input_digit(int digit)
{
    if (s_has_error)
    {
        calc_clear_all();
    }

    if (s_new_num)
    {
        s_current = digit;
        s_new_num = false;
    }
    else
    {
        if (s_current >= 0 && s_current < 100000000000000LL)
        {
            s_current = s_current * 10 + digit;
        }
        else if (s_current < 0 && s_current > -100000000000000LL)
        {
            s_current = s_current * 10 - digit;
        }
    }
}

static void calc_input_double_zero(void)
{
    if (s_has_error)
    {
        calc_clear_all();
        return;
    }

    if (s_new_num)
    {
        s_current = 0;
        s_new_num = true;
    }
    else
    {
        if (s_current >= 0 && s_current < 10000000000000LL)
        {
            s_current = s_current * 100;
        }
        else if (s_current < 0 && s_current > -10000000000000LL)
        {
            s_current = s_current * 100;
        }
    }
}

static void calc_input_negate(void)
{
    if (s_has_error)
        return;
    s_current = -s_current;
}

static void calc_input_del(void)
{
    if (s_has_error)
    {
        calc_clear_all();
        return;
    }

    if (s_new_num)
    {
        s_current = 0;
        return;
    }

    s_current /= 10;
}

static void calc_execute_op(void)
{
    if (s_op == OP_NONE)
        return;

    int64_t result = 0;
    switch (s_op)
    {
        case OP_ADD:
            result = s_stored + s_current;
            break;
        case OP_SUB:
            result = s_stored - s_current;
            break;
        case OP_MUL:
            result = s_stored * s_current;
            break;
        case OP_DIV:
            if (s_current == 0)
            {
                s_has_error = true;
                k_strcpy(s_error_text, "ERR: DIV/0");
                return;
            }
            result = s_stored / s_current;
            break;
        case OP_MOD:
            if (s_current == 0)
            {
                s_has_error = true;
                k_strcpy(s_error_text, "ERR: MOD/0");
                return;
            }
            result = s_stored % s_current;
            break;
        default:
            return;
    }

    s_current = result;
}

static void calc_input_op(calc_op_t op, char op_char)
{
    if (s_has_error)
    {
        calc_clear_all();
    }

    if (s_op != OP_NONE && !s_new_num)
    {
        calc_execute_op();
        if (s_has_error)
            return;
    }

    s_stored = s_current;
    s_op = op;
    s_new_num = true;

    char buf[32];
    format_int64(s_stored, buf);
    k_strcpy(s_expr_text, buf);
    int len = k_strlen(s_expr_text);
    s_expr_text[len++] = ' ';
    s_expr_text[len++] = op_char;
    s_expr_text[len] = '\0';
}

static void calc_input_equals(void)
{
    if (s_has_error || s_op == OP_NONE)
        return;

    char op_char = '?';
    switch (s_op)
    {
        case OP_ADD: op_char = '+'; break;
        case OP_SUB: op_char = '-'; break;
        case OP_MUL: op_char = '*'; break;
        case OP_DIV: op_char = '/'; break;
        case OP_MOD: op_char = '%'; break;
        default: break;
    }

    char s1[32], s2[32];
    format_int64(s_stored, s1);
    format_int64(s_current, s2);

    calc_execute_op();

    if (s_has_error)
    {
        k_strcpy(s_expr_text, "Cannot divide by 0");
        return;
    }

    k_strcpy(s_expr_text, s1);
    int p = k_strlen(s_expr_text);
    s_expr_text[p++] = ' ';
    s_expr_text[p++] = op_char;
    s_expr_text[p++] = ' ';
    k_strcpy(s_expr_text + p, s2);
    p = k_strlen(s_expr_text);
    s_expr_text[p++] = ' ';
    s_expr_text[p++] = '=';
    s_expr_text[p] = '\0';

    s_op = OP_NONE;
    s_stored = 0;
    s_new_num = true;
}

static bool calc_app_handle_key(Window *window, char c)
{
    (void)window;

    if (c >= '0' && c <= '9')
    {
        calc_input_digit(c - '0');
        return true;
    }
    else if (c == '+')
    {
        calc_input_op(OP_ADD, '+');
        return true;
    }
    else if (c == '-')
    {
        calc_input_op(OP_SUB, '-');
        return true;
    }
    else if (c == '*')
    {
        calc_input_op(OP_MUL, '*');
        return true;
    }
    else if (c == '/')
    {
        calc_input_op(OP_DIV, '/');
        return true;
    }
    else if (c == '%')
    {
        calc_input_op(OP_MOD, '%');
        return true;
    }
    else if (c == '=' || c == '\n')
    {
        calc_input_equals();
        return true;
    }
    else if (c == '\b')
    {
        calc_input_del();
        return true;
    }
    else if (c == 'c' || c == 'C')
    {
        calc_clear_all();
        return true;
    }
    else if (c == 'n' || c == 'N')
    {
        calc_input_negate();
        return true;
    }

    return false;
}

typedef struct
{
    const char *label;
    uint32_t bg_color;
    uint32_t border_color;
    uint32_t text_color;
} btn_style_t;

static const btn_style_t s_buttons[5][4] = {
    // Row 0
    {
        {"C",   0x25151D, 0x4D202D, 0xF87171},
        {"DEL", 0x182030, 0x2A384F, 0xCBD5E1},
        {"+/-", 0x142338, 0x213C5E, 0x38BDF8},
        {"/",   0x162A44, 0x254670, 0x38BDF8}
    },
    // Row 1
    {
        {"7", 0x151D2C, 0x243044, 0xF8FAFC},
        {"8", 0x151D2C, 0x243044, 0xF8FAFC},
        {"9", 0x151D2C, 0x243044, 0xF8FAFC},
        {"*", 0x162A44, 0x254670, 0x38BDF8}
    },
    // Row 2
    {
        {"4", 0x151D2C, 0x243044, 0xF8FAFC},
        {"5", 0x151D2C, 0x243044, 0xF8FAFC},
        {"6", 0x151D2C, 0x243044, 0xF8FAFC},
        {"-", 0x162A44, 0x254670, 0x38BDF8}
    },
    // Row 3
    {
        {"1", 0x151D2C, 0x243044, 0xF8FAFC},
        {"2", 0x151D2C, 0x243044, 0xF8FAFC},
        {"3", 0x151D2C, 0x243044, 0xF8FAFC},
        {"+", 0x162A44, 0x254670, 0x38BDF8}
    },
    // Row 4
    {
        {"%",  0x142338, 0x213C5E, 0x38BDF8},
        {"0",  0x151D2C, 0x243044, 0xF8FAFC},
        {"00", 0x151D2C, 0x243044, 0xF8FAFC},
        {"=",  0x0284C7, 0x38BDF8, 0x090D16}
    }
};

static bool calc_app_handle_mouse_down(Window *win, int x, int y)
{
    for (int row = 0; row < 5; row++)
    {
        int by = (win->y + 118) + row * 41;
        for (int col = 0; col < 4; col++)
        {
            int bx = (win->x + 12) + col * 60;
            if (x >= bx && x <= bx + 48 && y >= by && y <= by + 34)
            {
                if (row == 0)
                {
                    if (col == 0) calc_clear_all();
                    else if (col == 1) calc_input_del();
                    else if (col == 2) calc_input_negate();
                    else if (col == 3) calc_input_op(OP_DIV, '/');
                }
                else if (row == 1)
                {
                    if (col == 0) calc_input_digit(7);
                    else if (col == 1) calc_input_digit(8);
                    else if (col == 2) calc_input_digit(9);
                    else if (col == 3) calc_input_op(OP_MUL, '*');
                }
                else if (row == 2)
                {
                    if (col == 0) calc_input_digit(4);
                    else if (col == 1) calc_input_digit(5);
                    else if (col == 2) calc_input_digit(6);
                    else if (col == 3) calc_input_op(OP_SUB, '-');
                }
                else if (row == 3)
                {
                    if (col == 0) calc_input_digit(1);
                    else if (col == 1) calc_input_digit(2);
                    else if (col == 2) calc_input_digit(3);
                    else if (col == 3) calc_input_op(OP_ADD, '+');
                }
                else if (row == 4)
                {
                    if (col == 0) calc_input_op(OP_MOD, '%');
                    else if (col == 1) calc_input_digit(0);
                    else if (col == 2) calc_input_double_zero();
                    else if (col == 3) calc_input_equals();
                }

                gui_needs_redraw = true;
                return true;
            }
        }
    }

    return true;
}

void calc_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
    {
        return;
    }

    int x = window->x;
    int y = window->y;
    int w = window->width;
    int h = window->height;

    // Window interior background
    fill_rect(x + 2, y + TITLEBAR_HEIGHT, w - 4, h - TITLEBAR_HEIGHT - 2, 0x0B0F19);

    // Display Box (x + 12 to x + 240, width 228, height 68)
    fill_rect(x + 13, y + 41, 226, 66, 0x060911);
    fill_rect(x + 12, y + 40, 228, 1, 0x1E293B);
    fill_rect(x + 12, y + 107, 228, 1, 0x1E293B);
    fill_rect(x + 12, y + 40, 1, 68, 0x1E293B);
    fill_rect(x + 239, y + 40, 1, 68, 0x1E293B);

    // Top line in display: expression history or brand
    if (s_expr_text[0] != '\0')
    {
        draw_text_scaled(x + 20, y + 46, s_expr_text, GUI_TEXT_MUTED, 1, 1);
    }
    else
    {
        draw_text_scaled(x + 20, y + 46, "ZENITH MATH-64", 0x334155, 1, 1);
    }

    // Main readout (right-aligned)
    if (s_has_error)
    {
        size_t err_w = font_text_width_scaled(s_error_text, 2, 1);
        int draw_x = (x + 12 + 228) - 10 - (int)err_w;
        draw_text_scaled(draw_x, y + 64, s_error_text, GUI_ACCENT_DANGER, 2, 1);
    }
    else
    {
        char num_buf[32];
        format_int64(s_current, num_buf);
        size_t num_len = k_strlen(num_buf);

        if (num_len <= 12)
        {
            size_t val_w = font_text_width_scaled(num_buf, 2, 1);
            int draw_x = (x + 12 + 228) - 10 - (int)val_w;
            draw_text_scaled(draw_x, y + 64, num_buf, GUI_TEXT_PRIMARY, 2, 1);
        }
        else
        {
            size_t val_w = font_text_width_scaled(num_buf, 1, 1);
            int draw_x = (x + 12 + 228) - 10 - (int)val_w;
            draw_text_scaled(draw_x, y + 70, num_buf, GUI_TEXT_PRIMARY, 1, 1);
        }
    }

    // Bottom line in display: Hex preview
    char hex_buf[32];
    format_hex64(s_current, hex_buf);
    draw_text_scaled(x + 20, y + 93, hex_buf, g_gui_accent, 1, 1);

    // Buttons
    for (int row = 0; row < 5; row++)
    {
        int by = (y + 118) + row * 41;
        for (int col = 0; col < 4; col++)
        {
            int bx = (x + 12) + col * 60;
            const btn_style_t *btn = &s_buttons[row][col];

            uint32_t bg = btn->bg_color;
            uint32_t border = btn->border_color;
            uint32_t txt = btn->text_color;

            // Equals button dynamically uses system accent theme
            if (row == 4 && col == 3)
            {
                bg = g_gui_accent;
                border = g_gui_accent;
                txt = 0x090D16;
            }

            fill_rect(bx + 1, by + 1, 46, 32, bg);
            fill_rect(bx, by, 48, 1, border);
            fill_rect(bx, by + 33, 48, 1, border);
            fill_rect(bx, by, 1, 34, border);
            fill_rect(bx + 47, by, 1, 34, border);

            size_t tw = font_text_width_scaled(btn->label, 1, 1);
            int tx = bx + (48 - (int)tw) / 2;
            int ty = by + (34 - 8) / 2;
            draw_text_scaled(tx, ty, btn->label, txt, 1, 1);
        }
    }
}
