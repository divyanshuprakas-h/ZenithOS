#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef struct 
{
    volatile uint64_t locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif