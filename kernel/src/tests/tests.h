#ifndef TESTS_H
#define TESTS_H

void kernel_tests(void);

// void pmm_tests(void);
// void heap_tests(void);
// void vmm_tests(void);
// void cpu_tests(void);
// void driver_tests(void);
void test_atomic_xchg(void);
void test_spinlock(void);

#endif