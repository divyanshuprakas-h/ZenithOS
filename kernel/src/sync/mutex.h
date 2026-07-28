#ifndef MUTEX_H
#define MUTEX_H

#include <stdbool.h>

#include "irq_spinlock.h"
#include "../scheduler/task.h"

typedef struct task task_t;

typedef struct mutex
{
    irq_spinlock_t guard;
    bool locked;
    task_t *owner;
    task_t *wait_queue;
} mutex_t;

void mutex_init(mutex_t *mutex);

void mutex_lock(mutex_t *mutex);

void mutex_unlock(mutex_t *mutex);

#endif
