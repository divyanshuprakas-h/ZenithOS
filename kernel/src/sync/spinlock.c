#include "spinlock.h"
#include "../arch/x86_64/atomic.h"

void spinlock_init(spinlock_t *lock)
{
    lock->locked = 0;
}

void spin_lock(spinlock_t *lock)
{
    while (atomic_xchg_u64(&lock->locked, 1))
    {
        while (lock->locked)
        {
            cpu_pause();
        }
        
    }
}

void spin_unlock(spinlock_t *lock)
{
    memory_barrier();
    lock->locked = 0;
}
