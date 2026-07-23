#include "tests.h"

#include "../terminal/terminal.h"

#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/irq.h"

void cpu_tests(void)
{
    terminal_write("\n------------ CPU TESTS ------------\n");

    gdt_init();
    terminal_write("GDT OK\n");

    idt_init();
    terminal_write("IDT OK\n");

    irq_init();
    terminal_write("IRQ OK\n");

    terminal_write("CPU TESTS PASSED\n");
}