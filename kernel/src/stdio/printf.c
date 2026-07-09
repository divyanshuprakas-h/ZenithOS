#include "printf.h"
#include <stdarg.h>

#include "../terminal/terminal.h"
#include "../lib/convert.h"

void kprintf(const char *fmt, ...)
{
    va_list args;

    va_start (args, fmt);

    while (*fmt)
    {
        if (*fmt == '%')
        { 
            fmt++;

            if (*fmt == 'c')
            {
                char ch = (char)va_arg(args, int);
                terminal_putchar(ch);

                fmt++;
                continue;
            }

            if (*fmt == 'd')
            {
                int value = va_arg(args, int);
                char buffer[16];
                int_to_string(value, buffer);
                terminal_write(buffer);

                fmt++;
                continue;
            }


            if (*fmt == 's')
            {
                const char *str = va_arg(args, const char *);
                while (*str)
                {
                    terminal_putchar(*str);
                    str++;
                }

                fmt++;
                continue;
            }

            if (*fmt == 'x')
            {
                uint64_t value = (uint64_t)va_arg(args, unsigned int);
                char buffer[17];
                uint64_to_hex(value, buffer);
                terminal_write(buffer);

                fmt++;
                continue;
            }

            if (*fmt == 'u')
            {
                uint32_t value = va_arg(args, uint32_t);
                char buffer[16];
                uint_to_string(value, buffer);
                terminal_write(buffer);

                fmt++;
                continue;
            }

            if (*fmt == 'p')
            {
                uintptr_t value = (uintptr_t)va_arg(args, void *);

                char buffer[17];

                uint64_to_hex((uint64_t)value, buffer);

                terminal_write("0x");
                terminal_write(buffer);

                fmt++;
                continue;
            }

        }

        terminal_putchar(*fmt);
        fmt++;
    }

    va_end(args);
}