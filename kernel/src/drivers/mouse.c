#include "mouse.h"

#include "../cpu/irq.h"
#include "../hal/io.h"
#include "../hal/ports.h"

static void mouse_irq_handler(interrupt_context_t *context)
{
    (void)context;
    mouse_handler();
}

void mouse_init(void)
{
    irq_register_handler(IRQ_MOUSE_LINE, mouse_irq_handler);
}

void mouse_handler(void)
{
    uint8_t status = inb(KEYBOARD_STATUS);

    if ((status & 0x21u) != 0x21u)
    {
        return;
    }

    (void)inb(KEYBOARD_DATA);
}
