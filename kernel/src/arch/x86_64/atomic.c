#include "atomic.h"

void memory_barrier(void)
{
    __asm__ volatile("" ::: "memory");
}

