#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "windows.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct app_info
{
    const char *id;
    const char *name;
    const char *description;
    const char *dock_label;
    uint32_t icon_accent;
    void (*draw_icon)(int x, int y, int size);
    Window *(*get_window)(void);
    void (*toggle)(void);
    bool in_dock;
    bool in_launcher;
} app_info_t;

void app_manager_init(void);
size_t app_manager_get_count(void);
app_info_t *app_manager_get(size_t index);
app_info_t *app_manager_find(const char *id);
void app_manager_toggle_app(app_info_t *app);

#endif
