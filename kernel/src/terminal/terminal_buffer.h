#ifndef TERMINAL_BUFFER_H
#define TERMINAL_BUFFER_H

#include <stdint.h>

#define TERM_COLS 80
#define TERM_ROWS 50

typedef struct 
{
    char character;
    uint32_t foreground;
    uint32_t background;
} terminal_cell_t;

#endif