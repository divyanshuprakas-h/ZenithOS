#include "printf.h"
#include <stdarg.h>

#include "../terminal/terminal.h"
#include "../hal/io.h"
#include "../lib/convert.h"

static inline void kprintf_putc(char ch)
{
    terminal_putchar(ch);

#if defined(__x86_64__)
    outb(0xE9, (uint8_t)ch);
#endif
}

static void kprintf_puts(const char *str)
{
    while (*str)
    {
        kprintf_putc(*str);
        str++;
    }
}

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
                kprintf_putc(ch);

                fmt++;
                continue;
            }

            if (*fmt == 'd')
            {
                int value = va_arg(args, int);
                char buffer[16];
                int_to_string(value, buffer);
                kprintf_puts(buffer);

                fmt++;
                continue;
            }


            if (*fmt == 's')
            {
                const char *str = va_arg(args, const char *);
                kprintf_puts(str);

                fmt++;
                continue;
            }

            if (*fmt == 'l')
            {
                fmt++;

                if (*fmt == 'l')
                {
                    fmt++;
                }

                if (*fmt == 'x')
                {
                    uint64_t value = va_arg(args, uint64_t);

                    char buffer[17];
                    uint64_to_hex(value, buffer);

                    kprintf_puts(buffer);

                    fmt++;
                    continue;
                }
            }

            if (*fmt == 'x')
            {
                unsigned int value = va_arg(args, unsigned int);

                char buffer[17];
                uint64_to_hex((uint64_t)value, buffer);

                kprintf_puts(buffer);

                fmt++;
                continue;
            }

            if (*fmt == 'u')
            {
                uint32_t value = va_arg(args, uint32_t);
                char buffer[16];
                uint_to_string(value, buffer);
                kprintf_puts(buffer);

                fmt++;
                continue;
            }

            if (*fmt == 'p')
            {
                uintptr_t value = (uintptr_t)va_arg(args, void *);
                char buffer[17];

                uint64_to_hex((uint64_t)value, buffer);

                kprintf_puts("0x");
                kprintf_puts(buffer);

                fmt++;
                continue;
            }

        }

        kprintf_putc(*fmt);
        fmt++;
    }

    va_end(args);

    terminal_render();
}
