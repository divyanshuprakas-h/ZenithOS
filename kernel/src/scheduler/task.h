#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "context.h"

typedef enum{
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} task_state_t;

typedef struct task
{
    uint64_t id;

    cpu_context_t context;

    void (*entry)(void);

    bool started;

    void *kernel_stack;

    size_t kernel_stack_size;

    task_state_t state;

    struct task *next;

    uint64_t wake_tick;

} task_t;

task_t *task_create(void(*entry)(void));

#endif

