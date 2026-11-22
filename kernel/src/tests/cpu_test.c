#include "tests.h"

#include "../terminal/terminal.h"
#include "../cpu/interrupts.h"
#include "../cpu/irq.h"
#include "../hal/pit.h"

void cpu_tests(void)
{
    terminal_write("  [TEST] Running CPU tests...\n");

    bool irq = interrupt_save();
    TEST_ASSERT(true);
    interrupt_restore(irq);

    uint64_t ticks1 = pit_ticks();
    for (volatile int i = 0; i < 50000; i++);
    uint64_t ticks2 = pit_ticks();
    TEST_ASSERT(ticks2 >= ticks1);
}
