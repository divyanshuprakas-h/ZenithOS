#include "exceptions_test.h"

void test_divide_error(void)
{
    volatile int x = 1;
    volatile int y = 0;

    volatile int z = x / y;

    (void)z;
}

void test_invalid_opcode(void)
{
    __asm__ volatile("ud2");
}