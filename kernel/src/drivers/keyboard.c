#include "keyboard.h"

#include "../cpu/irq.h"
#include "../hal/io.h"
#include "../hal/pic.h"
#include "../hal/ports.h"
#include "scancode.h"
#include "../terminal/terminal.h"
#include "../gui/window_manager.h"
#include "../gui/gui_state.h"

static bool shift_pressed = false;

static void keyboard_irq_handler(interrupt_context_t *context)
{
    (void)context;
    keyboard_handler();
}

void keyboard_init(void)
{
    shift_pressed = false;
    irq_register_handler(IRQ_KEYBOARD_LINE, keyboard_irq_handler);
    pic_unmask_irq(IRQ_KEYBOARD_LINE);
}

void keyboard_handler(void)
{
    uint8_t scancode = inb(KEYBOARD_DATA);

    // Left shift (0x2A) or Right shift (0x36) press
    if (scancode == 0x2A || scancode == 0x36)
    {
        shift_pressed = true;
        return;
    }

    // Left shift or Right shift release (break code = make | 0x80)
    if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80))
    {
        shift_pressed = false;
        return;
    }

    // Ignore other key release events
    if ((scancode & 0x80u) != 0u)
    {
        return;
    }

    char c = scancode_to_ascii(scancode, shift_pressed);

    if (c != 0)
    {
        Window *focused = window_manager_get_focused();
        if (focused != NULL && focused->visible && !focused->minimised && focused->handle_key != NULL)
        {
            if (focused->handle_key(focused, c))
            {
                gui_needs_redraw = true;
                return;
            }
        }
        terminal_handle_key(c);
        gui_needs_redraw = true;
    }
}
