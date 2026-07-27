#ifndef IRQ_SPINLOCK_H
#define IRQ_SPINLOCK_H

#include "spinlock.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct 
{
    spinlock_t lock;
    bool irq_was_enabled;
} irq_spinlock_t;

void irq_spinlock_init(irq_spinlock_t *lock);

void irq_spin_lock(irq_spinlock_t *lock);

void irq_spin_unlock(irq_spinlock_t *lock);

#endif

