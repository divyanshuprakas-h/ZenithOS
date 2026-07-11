#include "pit.h"

#include "../cpu/irq.h"
#include "io.h"
#include "pic.h"

static volatile uint64_t system_ticks;

static void pit_program(uint32_t frequency_hz)
{
    if (frequency_hz == 0)
    {
        frequency_hz = PIT_DEFAULT_FREQUENCY_HZ;
    }

    uint32_t divisor = 1193182u / frequency_hz;

    if (divisor == 0)
    {
        divisor = 1;
    }

    outb(PIT_COMMAND, PIT_MODE_SQUARE);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
}

static void pit_irq_handler(interrupt_context_t *context)
{
    (void)context;
    system_ticks++;
}

void pit_init(uint32_t frequency_hz)
{
    pit_program(frequency_hz);
    irq_register_handler(IRQ_TIMER_LINE, pit_irq_handler);
    pic_unmask_irq(IRQ_TIMER_LINE);
}

uint64_t pit_ticks(void)
{
    return system_ticks;
}
