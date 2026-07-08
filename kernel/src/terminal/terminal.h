#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

void terminal_init(void);

void terminal_write(const char *str);

void terminal_putchar(char c);

void terminal_set_color(uint32_t color);

#endif
