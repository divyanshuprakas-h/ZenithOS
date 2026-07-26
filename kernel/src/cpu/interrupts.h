#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <stdbool.h>

static inline void interrupt_disable(void)
{
    __asm__ volatile("cli" ::: "memory");
}

static inline void interrupt_enable(void)
{
    __asm__ volatile("sti" ::: "memory");
}

static inline bool interrupts_enabled(void)
{
    uint64_t flags;

    __asm__ volatile(
        "pushfq\n\t"
        "pop %0"
        : "=r"(flags)
    );

    return (flags & (1ULL << 9)) != 0;
}

static inline bool interrupt_save(void)
{
    bool enabled = interrupts_enabled();
    interrupt_disable();
    return enabled;
}

static inline void interrupt_restore(bool enabled)
{
    if (enabled)
    {
        interrupt_enable();
    }
}

#endif
