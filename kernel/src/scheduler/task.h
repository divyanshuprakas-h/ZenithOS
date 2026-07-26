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
    TASK_TERMINATED,
    TASK_IDLE
} task_state_t;

typedef enum
{
    TASK_RESUME_CONTEXT,
    TASK_RESUME_INTERRUPT
} task_resume_mode_t;

typedef struct task
{
    uint64_t id;

    cpu_context_t context;

    void *interrupt_rsp;

    void (*entry)(void);

    void *kernel_stack;

    size_t kernel_stack_size;

    task_state_t state;

    struct task *next;

    uint64_t wake_tick;

    task_resume_mode_t resume_mode;

} task_t;

task_t *task_create(void(*entry)(void));

static inline bool task_resume_uses_interrupt_frame(const task_t *task)
{
    return task != NULL &&
           task->resume_mode == TASK_RESUME_INTERRUPT &&
           task->interrupt_rsp != NULL;
}

#endif
