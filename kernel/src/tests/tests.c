#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"

uint32_t g_tests_run = 0;
uint32_t g_tests_passed = 0;
uint32_t g_tests_failed = 0;

void kernel_tests(void)
{
    g_tests_run = 0;
    g_tests_passed = 0;
    g_tests_failed = 0;

    terminal_write("\n=====================================\n");
    terminal_write("        ZENITHOS KERNEL TESTS\n");
    terminal_write("=====================================\n");

    // 1. Core memory & paging tests
    pmm_tests();
    heap_tests();
    vmm_tests();

    // 2. CPU & synchronization tests
    cpu_tests();
    test_atomic_xchg();
    test_spinlock();
    test_irq_spinlock();
    test_mutex_init();
    test_mutex_lock();

    // 3. Drivers
    driver_tests();

    // 4. Reliability & safety regression tests
    test_user_access_regression();
    test_syscall_errors_regression();
    test_dirent_abi_regression();
    test_fd_cleanup_regression();
    test_elf_validation_regression();
    test_telemetry_regression();

    // Summary report
    terminal_write("\n=====================================\n");
    kprintf("TEST RESULTS: Total: %u | Passed: %u | Failed: %u\n",
            (unsigned)g_tests_run, (unsigned)g_tests_passed, (unsigned)g_tests_failed);

    if (g_tests_failed == 0)
    {
        terminal_write("STATUS: ALL TESTS PASSED\n");
    }
    else
    {
        terminal_write("STATUS: WARNING - SOME TESTS FAILED\n");
    }
    terminal_write("=====================================\n\n");
}
