#include "irq.h"

#include "../hal/pic.h"

static irq_handler_t irq_handlers[IRQ_COUNT];

void irq_init(void)
{
    for (uint8_t line = 0; line < IRQ_COUNT; ++line)
    {
        irq_handlers[line] = 0;
    }
}

bool irq_register_handler(uint8_t irq_line, irq_handler_t handler)
{
    if (irq_line >= IRQ_COUNT)
    {
        return false;
    }

    irq_handlers[irq_line] = handler;
    return true;
}

void irq_dispatch(interrupt_context_t *context)
{
    uint64_t interrupt_number = context->interrupt_number;

    if (interrupt_number < IRQ_BASE_VECTOR ||
        interrupt_number >= IRQ_BASE_VECTOR + IRQ_COUNT)
    {
        return;
    }

    uint8_t irq_line = (uint8_t)(interrupt_number - IRQ_BASE_VECTOR);
    irq_handler_t handler = irq_handlers[irq_line];

    if (handler != 0)
    {
        handler(context);
    }

    pic_send_eoi(irq_line);
}
