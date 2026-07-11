#ifndef IRQ_H
#define IRQ_H

#include <stdbool.h>
#include <stdint.h>

#include "interrupt_context.h"

enum
{
    IRQ_BASE_VECTOR = 32,
    IRQ_COUNT = 16,
    IRQ_TIMER_LINE = 0,
    IRQ_KEYBOARD_LINE = 1,
    IRQ_MOUSE_LINE = 12
};

typedef void (*irq_handler_t)(interrupt_context_t *context);

void irq_init(void);
bool irq_register_handler(uint8_t irq_line, irq_handler_t handler);
void irq_dispatch(interrupt_context_t *context);

#endif
