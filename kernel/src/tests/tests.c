#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"

void kernel_tests(void)
{
    kprintf("[TRACE] kernel_tests entry\n");
    terminal_write("\n");
    terminal_write("=====================================\n");
    terminal_write("        ZENITHOS KERNEL TESTS\n");
    terminal_write("=====================================\n\n");

    // pmm_tests();

    // heap_tests();

    // vmm_tests();

    // cpu_tests();

    // driver_tests();

    kprintf("[TRACE] test_atomic_xchg begin\n");
    test_atomic_xchg();
    kprintf("[TRACE] test_atomic_xchg end\n");

    kprintf("[TRACE] test_spinlock begin\n");
    test_spinlock();
    kprintf("[TRACE] test_spinlock end\n");

    kprintf("[TRACE] test_irq_spinlock begin\n");
    test_irq_spinlock();
    kprintf("[TRACE] test_irq_spinlock end\n");

    kprintf("[TRACE] test_mutex_init begin\n");
    test_mutex_init();
    kprintf("[TRACE] test_mutex_init end\n");

    kprintf("[TRACE] test_mutex_lock begin\n");
    test_mutex_lock();
    kprintf("[TRACE] test_mutex_lock end\n");

    terminal_write("\n");
    terminal_write("=====================================\n");
    terminal_write("      ALL TESTS PASSED\n");
    terminal_write("=====================================\n\n");
    kprintf("[TRACE] kernel_tests exit\n");

    kprintf("[TRACE] kernel_tests exit\n");

}
