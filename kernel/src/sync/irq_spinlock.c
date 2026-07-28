#include "irq_spinlock.h"
#include "../arch/x86_64/interrupt.h"

void irq_spinlock_init(irq_spinlock_t *lock)
{
    spinlock_init(&lock->lock);
    lock->irq_was_enabled = false;
}

void irq_spin_lock(irq_spinlock_t *lock)
{
    lock->irq_was_enabled = cpu_interrupts_enabled();
    cpu_cli();
    spin_lock(&lock->lock);
}

void irq_spin_unlock(irq_spinlock_t *lock)
{
    spin_unlock(&lock->lock);

    if (lock->irq_was_enabled)
    {
        cpu_sti();
    }
}
