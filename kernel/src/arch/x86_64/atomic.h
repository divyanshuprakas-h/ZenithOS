#ifndef ARCH_X86_64_ATOMIC_H
#define ARCH_X86_64_ATOMIC_H

#include <stdint.h>
#include <stdbool.h>

uint64_t atomic_xchg_u64(volatile uint64_t *ptr, uint64_t value);

bool atomic_cmpxchg_u64(
    volatile uint64_t *ptr,
    uint64_t expected,
    uint64_t desired
);

uint64_t atomic_fetch_add_u64(
    volatile uint64_t *ptr,
    uint64_t value
);

uint64_t atomic_fetch_sub_u64(
    volatile uint64_t *ptr,
    uint64_t value
);

void cpu_pause(void);

void memory_barrier(void);

#endif