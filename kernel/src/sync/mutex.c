#include "mutex.h"

#include "../scheduler/queue.h"
#include "../scheduler/scheduler.h"


void mutex_init(mutex_t *mutex)
{
    irq_spinlock_init(&mutex->guard);
    mutex->locked = false;
    mutex->owner = NULL;
    mutex->wait_queue = NULL;
}

void mutex_lock(mutex_t *mutex)
{
    task_t *current = scheduler_current_task();

    if (mutex == NULL || current == NULL)
    {
        return;
    }

    if (mutex->owner == current)
    {
        return;
    }

    while (1)
    {
        irq_spin_lock(&mutex->guard);

        if (mutex->owner == current)
        {
            irq_spin_unlock(&mutex->guard);
            return;
        }

        if (!mutex->locked)
        {
            mutex->locked = true;
            mutex->owner = current;

            irq_spin_unlock(&mutex->guard);
            return;
        }

        current->state = TASK_BLOCKED;
        task_queue_push(&mutex->wait_queue, current);

        irq_spin_unlock(&mutex->guard);

        scheduler_yield();
    }
}

void mutex_unlock(mutex_t *mutex)
{
    task_t *current = scheduler_current_task();

    if (mutex == NULL || current == NULL)
    {
        return;
    }

    irq_spin_lock(&mutex->guard);

    if (mutex->owner != current)
    {
        irq_spin_unlock(&mutex->guard);
        return;
    }

    if (mutex->wait_queue == NULL)
    {
        mutex->locked = false;
        mutex->owner = NULL;

        irq_spin_unlock(&mutex->guard);
        return;
    }

    task_t *next = mutex->wait_queue;

    task_queue_remove(&mutex->wait_queue, next);

    mutex->owner = next;

    scheduler_unblock(next);

    irq_spin_unlock(&mutex->guard);
}
