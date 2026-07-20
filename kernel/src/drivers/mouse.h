#ifndef MOUSE_H
#define MOUSE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    int x;
    int y;

    bool left;
    bool right;
    bool middle;

} MouseState;

extern MouseState mouse_state;

void mouse_init(void);
void mouse_handler(void);

#endif
