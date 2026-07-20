#include "keyboard.h"

#include "../cpu/irq.h"
#include "../hal/io.h"
#include "../hal/pic.h"
#include "../hal/ports.h"
#include "scancode.h"
#include "../terminal/terminal.h"

static void keyboard_irq_handler(interrupt_context_t *context)
{
    (void)context;
    keyboard_handler();
}

void keyboard_init(void)
{
    irq_register_handler(IRQ_KEYBOARD_LINE, keyboard_irq_handler);
    pic_unmask_irq(IRQ_KEYBOARD_LINE);
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA);

    if ((scancode & 0x80u) != 0u)
    {
        return;
    }

    char c = scancode_to_ascii(scancode);

    if (c == '\b')
    {
        terminal_backspace();
        return;
    }

    if (c != 0)
    {
        terminal_handle_key(c);
    }
}

