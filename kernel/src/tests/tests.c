#include "tests.h"

#include "../terminal/terminal.h"

void kernel_tests(void)
{
    terminal_write("\n");
    terminal_write("=====================================\n");
    terminal_write("        ZENITHOS KERNEL TESTS\n");
    terminal_write("=====================================\n\n");

    pmm_tests();

    heap_tests();

    vmm_tests();

    cpu_tests();

    driver_tests();

    terminal_write("\n");
    terminal_write("=====================================\n");
    terminal_write("      ALL TESTS PASSED\n");
    terminal_write("=====================================\n\n");
}