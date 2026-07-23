#ifndef WINDOWS_H
#define WINDOWS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Window Window;

struct Window
{
    int x;
    int y;

    int width;
    int height;

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
};

void window_draw(Window *window);

bool window_in_titlebar(Window *window, int x, int y);

#endif