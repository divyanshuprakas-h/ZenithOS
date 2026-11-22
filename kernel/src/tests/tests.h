#ifndef TESTS_H
#define TESTS_H

#include <stdint.h>
#include <stdbool.h>
#include "../stdio/printf.h"

extern uint32_t g_tests_run;
extern uint32_t g_tests_passed;
extern uint32_t g_tests_failed;

void kernel_tests(void);

#define TEST_ASSERT(expr) do { \
    g_tests_run++; \
    if (expr) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        kprintf("[TEST FAIL] %s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
    } \
} while (0)

#define TEST_ASSERT_MSG(expr, msg) do { \
    g_tests_run++; \
    if (expr) { \
        g_tests_passed++; \
    } else { \
        g_tests_failed++; \
        kprintf("[TEST FAIL] %s:%d: %s (%s)\n", __FILE__, __LINE__, msg, #expr); \
    } \
} while (0)

void pmm_tests(void);
void heap_tests(void);
void vmm_tests(void);
void cpu_tests(void);
void driver_tests(void);

void test_atomic_xchg(void);
void test_spinlock(void);
void test_irq_spinlock(void);
void test_mutex_init(void);
void test_mutex_lock(void);

void test_user_access_regression(void);
void test_syscall_errors_regression(void);
void test_dirent_abi_regression(void);
void test_fd_cleanup_regression(void);
void test_elf_validation_regression(void);
void test_telemetry_regression(void);

#endif
