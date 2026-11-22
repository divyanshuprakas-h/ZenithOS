#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Window Window;

typedef enum
{
    WINDOW_CONTROL_NONE = 0,
    WINDOW_CONTROL_CLOSE,
    WINDOW_CONTROL_MINIMIZE,
    WINDOW_CONTROL_MAXIMIZE
} window_control_t;

struct Window
{
    int x;
    int y;
    int width;
    int height;

    int saved_x;
    int saved_y;
    int saved_width;
    int saved_height;

    bool visible;
    bool focused;

    bool minimised;
    bool maximised;

    bool dragging;
    bool resizing;

    int drag_offset_x;
    int drag_offset_y;

    char title[64];

    void (*draw_content)(Window *);
    void (*on_close)(Window *);
};

void window_draw(Window *window);
bool window_in_titlebar(Window *window, int x, int y);
window_control_t window_hit_test_control(Window *window, int x, int y);

#endif
