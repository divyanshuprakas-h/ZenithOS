#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

void terminal_init(void);

void terminal_write(const char *str);

void terminal_putchar(char c);

void terminal_backspace(void);

void terminal_set_color(uint32_t color);

void terminal_set_origin(int x, int y);

void terminal_handle_key(char c);

void terminal_render(void);

const char *terminal_get_input(void);

#endif
