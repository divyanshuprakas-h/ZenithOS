#include "files_app.h"
#include "../gui/windows.h"
#include "../gui/window_manager.h"
#include "../gui/theme.h"
#include "../graphics/graphics.h"
#include "../font/font.h"
#include "../fs/vfs.h"
#include "../lib/string.h"
#include "../kernel_err/errno.h"

#define MAX_ENTRIES 32

static char s_current_path[128] = "/";
static vfs_dirent_t s_entries[MAX_ENTRIES];
static size_t s_entry_count = 0;
static int s_selected_idx = -1;
static char s_preview_buf[512];
static size_t s_preview_len = 0;
static bool s_has_preview = false;
static bool s_is_binary = false;

static bool files_app_handle_mouse_down(Window *window, int x, int y);
static void files_app_on_close(Window *window);
static void files_app_load_dir(const char *path);

static Window files_window = {
    .x = 160,
    .y = 70,

    .width = 640,
    .height = 410,

    .saved_x = 160,
    .saved_y = 70,
    .saved_width = 640,
    .saved_height = 410,

    .visible = false,
    .focused = false,

    .minimised = false,
    .maximised = false,

    .dragging = false,
    .resizing = false,

    .title = "Files - /",

    .draw_content = files_app_draw,
    .on_close = files_app_on_close,
    .handle_mouse_down = files_app_handle_mouse_down
};

static void format_number(uint64_t val, char *buf)
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

static void files_app_on_close(Window *window)
{
    (void)window;
}

void files_app_init(void)
{
    files_app_load_dir("/");
    window_manager_add(&files_window);
}

Window *files_app_get_window(void)
{
    return &files_window;
}

bool files_app_is_focused(void)
{
    return files_window.visible && !files_window.minimised && files_window.focused;
}

void files_app_dock_click(void)
{
    if (!files_window.visible || files_window.minimised)
    {
        files_window.visible = true;
        files_window.minimised = false;
        window_manager_focus(&files_window);
    }
    else if (files_window.focused)
    {
        window_manager_minimize(&files_window);
    }
    else
    {
        window_manager_focus(&files_window);
    }
}

static void files_app_load_dir(const char *path)
{
    if (path == NULL)
        return;

    k_strncpy(s_current_path, path, sizeof(s_current_path) - 1);
    s_current_path[sizeof(s_current_path) - 1] = '\0';

    k_strcpy(files_window.title, "Files - ");
    k_strcat(files_window.title, s_current_path);

    s_entry_count = 0;
    s_selected_idx = -1;
    s_has_preview = false;
    s_is_binary = false;
    s_preview_len = 0;
    s_preview_buf[0] = '\0';

    vfs_read_dir(s_current_path, s_entries, MAX_ENTRIES, &s_entry_count);
    gui_needs_redraw = true;
}

static void files_app_open_file(size_t index)
{
    if (index >= s_entry_count)
        return;

    char full_path[160];
    k_strcpy(full_path, s_current_path);
    if (k_strcmp(s_current_path, "/") != 0)
    {
        k_strcat(full_path, "/");
    }
    k_strcat(full_path, s_entries[index].name);

    file_t *file = NULL;
    int rc = vfs_open(full_path, &file);

    s_has_preview = true;
    s_preview_len = 0;
    s_is_binary = false;
    s_preview_buf[0] = '\0';

    if (rc == KERNEL_SUCCESS && file != NULL)
    {
        int bytes = vfs_read(file, s_preview_buf, sizeof(s_preview_buf) - 1);
        if (bytes > 0)
        {
            s_preview_len = (size_t)bytes;
            s_preview_buf[bytes] = '\0';

            // Check if binary / ELF
            if (bytes >= 4 && s_preview_buf[0] == 0x7F && s_preview_buf[1] == 'E' &&
                s_preview_buf[2] == 'L' && s_preview_buf[3] == 'F')
            {
                s_is_binary = true;
            }
            else
            {
                for (int i = 0; i < bytes; i++)
                {
                    unsigned char c = (unsigned char)s_preview_buf[i];
                    if (c < 32 && c != '\n' && c != '\r' && c != '\t')
                    {
                        s_is_binary = true;
                        break;
                    }
                }
            }
        }
        vfs_close(file);
    }
    gui_needs_redraw = true;
}

static void files_app_go_up(void)
{
    if (k_strcmp(s_current_path, "/") == 0)
        return;

    size_t len = k_strlen(s_current_path);
    if (len <= 1)
        return;

    // Find last '/'
    int last_slash = -1;
    for (int i = (int)len - 1; i >= 0; i--)
    {
        if (s_current_path[i] == '/')
        {
            last_slash = i;
            break;
        }
    }

    if (last_slash <= 0)
    {
        files_app_load_dir("/");
    }
    else
    {
        s_current_path[last_slash] = '\0';
        files_app_load_dir(s_current_path);
    }
}

static bool files_app_handle_mouse_down(Window *win, int x, int y)
{
    int wx = win->x;
    int wy = win->y;

    // Check [Up /..] button: wx + 14, wy + 40, w=64, h=24
    if (x >= wx + 14 && x <= wx + 14 + 64 && y >= wy + 40 && y <= wy + 40 + 24)
    {
        files_app_go_up();
        return true;
    }

    // Check left pane items: list starts at wy + 102
    int list_x = wx + 14;
    int list_y = wy + 102;
    int list_w = 340;
    int row_h = 24;

    if (x >= list_x && x <= list_x + list_w && y >= list_y)
    {
        int rel_y = y - list_y;
        size_t clicked_idx = (size_t)(rel_y / row_h);
        if (clicked_idx < s_entry_count)
        {
            if (s_entries[clicked_idx].type == VNODE_DIRECTORY)
            {
                char new_path[140];
                k_strcpy(new_path, s_current_path);
                if (k_strcmp(s_current_path, "/") != 0)
                {
                    k_strcat(new_path, "/");
                }
                k_strcat(new_path, s_entries[clicked_idx].name);
                files_app_load_dir(new_path);
            }
            else
            {
                s_selected_idx = (int)clicked_idx;
                files_app_open_file(clicked_idx);
            }
            return true;
        }
    }

    return true;
}

void files_app_draw(Window *window)
{
    if (window == NULL || !window->visible || window->minimised)
        return;

    int wx = window->x;
    int wy = window->y;
    int ww = window->width;
    int wh = window->height;

    // Content background
    fill_rect(wx + 2, wy + TITLEBAR_HEIGHT, ww - 4, wh - TITLEBAR_HEIGHT - 2, 0x0B0F19);

    // Navigation Bar (wy + 36 to wy + 68)
    fill_rect(wx + 8, wy + 36, ww - 16, 32, 0x131A29);
    draw_rect(wx + 8, wy + 36, ww - 16, 32, 0x222E42);

    bool is_root = (k_strcmp(s_current_path, "/") == 0);
    fill_rect(wx + 14, wy + 40, 64, 24, is_root ? 0x182030 : 0x1E293B);
    draw_rect(wx + 14, wy + 40, 64, 24, is_root ? 0x222E42 : 0x38BDF8);
    draw_text_scaled(wx + 24, wy + 46, "^ Up", is_root ? GUI_TEXT_MUTED : GUI_ACCENT_PRIMARY, 1, 1);

    draw_text_scaled(wx + 88, wy + 46, "Path:", GUI_TEXT_MUTED, 1, 1);
    draw_text_scaled(wx + 130, wy + 46, s_current_path, GUI_TEXT_PRIMARY, 1, 1);

    char items_buf[32];
    format_number(s_entry_count, items_buf);
    k_strcat(items_buf, " items");
    size_t items_w = font_text_width_scaled(items_buf, 1, 1);
    draw_text_scaled(wx + ww - (int)items_w - 20, wy + 46, items_buf, GUI_TEXT_MUTED, 1, 1);

    // Left Pane: File List
    int list_x = wx + 12;
    int list_y = wy + 76;
    int list_w = 344;
    int list_h = wh - 86;

    fill_rect(list_x, list_y, list_w, list_h, 0x0E1422);
    draw_rect(list_x, list_y, list_w, list_h, 0x1E2A3E);

    // List Table Header
    fill_rect(list_x + 1, list_y + 1, list_w - 2, 24, 0x151E2E);
    draw_text_scaled(list_x + 12, list_y + 7, "NAME", GUI_TEXT_MUTED, 1, 1);
    draw_text_scaled(list_x + 190, list_y + 7, "TYPE", GUI_TEXT_MUTED, 1, 1);
    draw_text_scaled(list_x + 270, list_y + 7, "SIZE", GUI_TEXT_MUTED, 1, 1);

    int row_h = 24;
    for (size_t i = 0; i < s_entry_count && i < 12; i++)
    {
        int row_y = list_y + 26 + (int)i * row_h;
        bool selected = ((int)i == s_selected_idx);

        if (selected)
        {
            fill_rect(list_x + 2, row_y, list_w - 4, row_h - 1, 0x1E293B);
            draw_rect(list_x + 2, row_y, list_w - 4, row_h - 1, g_gui_accent);
        }
        else if (i % 2 == 1)
        {
            fill_rect(list_x + 2, row_y, list_w - 4, row_h - 1, 0x111827);
        }

        if (s_entries[i].type == VNODE_DIRECTORY)
        {
            // Folder icon badge [DIR]
            fill_rect(list_x + 8, row_y + 4, 18, 14, 0x78350F);
            draw_rect(list_x + 8, row_y + 4, 18, 14, 0xD97706);
            fill_rect(list_x + 11, row_y + 7, 12, 8, 0xF59E0B);

            draw_text_scaled(list_x + 32, row_y + 6, s_entries[i].name, 0xF59E0B, 1, 1);
            draw_text_scaled(list_x + 190, row_y + 6, "Folder", GUI_TEXT_MUTED, 1, 1);
            draw_text_scaled(list_x + 270, row_y + 6, "--", GUI_TEXT_MUTED, 1, 1);
        }
        else
        {
            // File icon badge [FILE]
            fill_rect(list_x + 8, row_y + 4, 18, 14, 0x0F324D);
            draw_rect(list_x + 8, row_y + 4, 18, 14, 0x0284C7);
            fill_rect(list_x + 11, row_y + 7, 12, 8, 0x38BDF8);

            draw_text_scaled(list_x + 32, row_y + 6, s_entries[i].name, GUI_TEXT_PRIMARY, 1, 1);
            draw_text_scaled(list_x + 190, row_y + 6, "File", GUI_TEXT_SECONDARY, 1, 1);

            char sz_buf[32];
            format_number(s_entries[i].size, sz_buf);
            k_strcat(sz_buf, " B");
            draw_text_scaled(list_x + 270, row_y + 6, sz_buf, GUI_TEXT_SECONDARY, 1, 1);
        }
    }

    if (s_entry_count == 0)
    {
        draw_text_scaled(list_x + 100, list_y + 60, "(Directory is empty)", GUI_TEXT_MUTED, 1, 1);
    }

    // Right Pane: Preview / Details
    int prev_x = wx + 366;
    int prev_y = wy + 76;
    int prev_w = ww - 378;
    int prev_h = wh - 86;

    fill_rect(prev_x, prev_y, prev_w, prev_h, 0x0E1422);
    draw_rect(prev_x, prev_y, prev_w, prev_h, 0x1E2A3E);

    // Preview Header
    fill_rect(prev_x + 1, prev_y + 1, prev_w - 2, 24, 0x151E2E);
    draw_text_scaled(prev_x + 12, prev_y + 7, "DETAILS & PREVIEW", GUI_TEXT_MUTED, 1, 1);

    if (s_selected_idx >= 0 && s_selected_idx < (int)s_entry_count)
    {
        vfs_dirent_t *sel = &s_entries[s_selected_idx];

        draw_text_scaled(prev_x + 14, prev_y + 36, "File:", GUI_TEXT_MUTED, 1, 1);
        draw_text_scaled(prev_x + 60, prev_y + 36, sel->name, g_gui_accent, 1, 1);

        char sz_buf[32];
        format_number(sel->size, sz_buf);
        k_strcat(sz_buf, " bytes");
        draw_text_scaled(prev_x + 14, prev_y + 54, "Size:", GUI_TEXT_MUTED, 1, 1);
        draw_text_scaled(prev_x + 60, prev_y + 54, sz_buf, GUI_TEXT_PRIMARY, 1, 1);

        draw_line(prev_x + 12, prev_y + 72, prev_x + prev_w - 12, prev_y + 72, 0x243248);

        if (s_is_binary)
        {
            fill_rect(prev_x + 14, prev_y + 84, prev_w - 28, 44, 0x162032);
            draw_rect(prev_x + 14, prev_y + 84, prev_w - 28, 44, 0x2A3B54);
            draw_text_scaled(prev_x + 22, prev_y + 92, "[Executable Binary File]", 0x38BDF8, 1, 1);
            draw_text_scaled(prev_x + 22, prev_y + 108, "ELF64 x86_64 format", GUI_TEXT_MUTED, 1, 1);
        }
        else if (s_has_preview)
        {
            draw_text_scaled(prev_x + 14, prev_y + 80, "Text Content:", GUI_TEXT_MUTED, 1, 1);

            int text_y = prev_y + 100;
            char line_buf[64];
            size_t line_pos = 0;

            for (size_t p = 0; p < s_preview_len && text_y < prev_y + prev_h - 20; p++)
            {
                char c = s_preview_buf[p];
                if (c == '\n' || line_pos >= sizeof(line_buf) - 1)
                {
                    line_buf[line_pos] = '\0';
                    draw_text_scaled(prev_x + 14, text_y, line_buf, GUI_TEXT_PRIMARY, 1, 1);
                    text_y += 16;
                    line_pos = 0;
                }
                else if (c != '\r')
                {
                    line_buf[line_pos++] = c;
                }
            }
            if (line_pos > 0 && text_y < prev_y + prev_h - 20)
            {
                line_buf[line_pos] = '\0';
                draw_text_scaled(prev_x + 14, text_y, line_buf, GUI_TEXT_PRIMARY, 1, 1);
            }
        }
    }
    else
    {
        draw_text_scaled(prev_x + 14, prev_y + 40, "No file selected.", GUI_TEXT_MUTED, 1, 1);
        draw_text_scaled(prev_x + 14, prev_y + 60, "Click a folder to browse,", GUI_TEXT_SECONDARY, 1, 1);
        draw_text_scaled(prev_x + 14, prev_y + 76, "or click a file to preview.", GUI_TEXT_SECONDARY, 1, 1);

        draw_line(prev_x + 12, prev_y + 104, prev_x + prev_w - 12, prev_y + 104, 0x1E2A3E);

        draw_text_scaled(prev_x + 14, prev_y + 116, "Filesystem Info:", GUI_TEXT_MUTED, 1, 1);
        draw_text_scaled(prev_x + 14, prev_y + 136, "Type: RAMFS In-Memory", GUI_TEXT_SECONDARY, 1, 1);
        draw_text_scaled(prev_x + 14, prev_y + 154, "Root: / (Mounted)", GUI_TEXT_SECONDARY, 1, 1);
        draw_text_scaled(prev_x + 14, prev_y + 172, "Status: VFS Read/Write", GUI_ACCENT_SUCCESS, 1, 1);
    }
}
