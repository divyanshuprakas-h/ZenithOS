#include "app_manager.h"
#include "../apps/launcher_app.h"
#include "../apps/terminal_app.h"
#include "../apps/files_app.h"
#include "../apps/calc_app.h"
#include "../apps/sysmon_app.h"
#include "../apps/settings_app.h"
#include "../apps/device_studio_app.h"
#include "../gui/window_manager.h"
#include "../lib/string.h"

static app_info_t s_apps[7];
static size_t s_app_count = 0;

void app_manager_init(void)
{
    s_app_count = 7;

    // 0: Applications Launcher
    s_apps[0].id = "apps";
    s_apps[0].name = "Applications";
    s_apps[0].description = "App Launcher";
    s_apps[0].dock_label = "APPS";
    s_apps[0].icon_accent = 0x38BDF8;
    s_apps[0].draw_icon = NULL;
    s_apps[0].get_window = launcher_app_get_window;
    s_apps[0].toggle = launcher_app_toggle;
    s_apps[0].in_dock = true;
    s_apps[0].in_launcher = false;

    // 1: Terminal
    s_apps[1].id = "terminal";
    s_apps[1].name = "Terminal";
    s_apps[1].description = "Ring 3 User Shell";
    s_apps[1].dock_label = "TERM";
    s_apps[1].icon_accent = 0x22C55E;
    s_apps[1].draw_icon = NULL;
    s_apps[1].get_window = terminal_app_get_window;
    s_apps[1].toggle = terminal_app_dock_click;
    s_apps[1].in_dock = true;
    s_apps[1].in_launcher = true;

    // 2: Files
    s_apps[2].id = "files";
    s_apps[2].name = "Files";
    s_apps[2].description = "VFS File Manager";
    s_apps[2].dock_label = "FILES";
    s_apps[2].icon_accent = 0xF59E0B;
    s_apps[2].draw_icon = NULL;
    s_apps[2].get_window = files_app_get_window;
    s_apps[2].toggle = files_app_dock_click;
    s_apps[2].in_dock = true;
    s_apps[2].in_launcher = true;

    // 3: Calculator
    s_apps[3].id = "calc";
    s_apps[3].name = "Calculator";
    s_apps[3].description = "System Math & Hex";
    s_apps[3].dock_label = "CALC";
    s_apps[3].icon_accent = 0x06B6D4;
    s_apps[3].draw_icon = NULL;
    s_apps[3].get_window = calc_app_get_window;
    s_apps[3].toggle = calc_app_dock_click;
    s_apps[3].in_dock = true;
    s_apps[3].in_launcher = true;

    // 4: System Monitor
    s_apps[4].id = "sysmon";
    s_apps[4].name = "System Monitor";
    s_apps[4].description = "RAM & System Stats";
    s_apps[4].dock_label = "SYS";
    s_apps[4].icon_accent = 0x38BDF8;
    s_apps[4].draw_icon = NULL;
    s_apps[4].get_window = sysmon_app_get_window;
    s_apps[4].toggle = sysmon_app_dock_click;
    s_apps[4].in_dock = true;
    s_apps[4].in_launcher = true;

    // 5: Settings
    s_apps[5].id = "settings";
    s_apps[5].name = "Settings";
    s_apps[5].description = "Theme & OS Config";
    s_apps[5].dock_label = "SET";
    s_apps[5].icon_accent = 0xA855F7;
    s_apps[5].draw_icon = NULL;
    s_apps[5].get_window = settings_app_get_window;
    s_apps[5].toggle = settings_app_dock_click;
    s_apps[5].in_dock = true;
    s_apps[5].in_launcher = true;

    // 6: Device Studio
    s_apps[6].id = "studio";
    s_apps[6].name = "Device Studio";
    s_apps[6].description = "Robotics & Telemetry";
    s_apps[6].dock_label = "DEV";
    s_apps[6].icon_accent = 0x10B981;
    s_apps[6].draw_icon = NULL;
    s_apps[6].get_window = device_studio_app_get_window;
    s_apps[6].toggle = device_studio_app_dock_click;
    s_apps[6].in_dock = true;
    s_apps[6].in_launcher = true;
}

size_t app_manager_get_count(void)
{
    return s_app_count;
}

app_info_t *app_manager_get(size_t index)
{
    if (index >= s_app_count)
        return NULL;
    return &s_apps[index];
}

app_info_t *app_manager_find(const char *id)
{
    if (id == NULL)
        return NULL;
    for (size_t i = 0; i < s_app_count; i++)
    {
        if (k_strcmp(s_apps[i].id, id) == 0)
            return &s_apps[i];
    }
    return NULL;
}

void app_manager_toggle_app(app_info_t *app)
{
    if (app == NULL)
        return;
    if (app->toggle != NULL)
    {
        app->toggle();
        return;
    }
    if (app->get_window != NULL)
    {
        Window *win = app->get_window();
        if (win == NULL)
            return;
        if (!win->visible || win->minimised)
        {
            win->visible = true;
            win->minimised = false;
            window_manager_focus(win);
        }
        else if (win->focused)
        {
            window_manager_minimize(win);
        }
        else
        {
            window_manager_focus(win);
        }
    }
}
