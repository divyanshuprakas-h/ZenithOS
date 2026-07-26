#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>

struct task;
typedef struct task task_t;

typedef struct cpu_context
{
    uint64_t rsp;

    uint64_t rbx;
    uint64_t rbp;

    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

} cpu_context_t;

void context_init(
    cpu_context_t *context,
    void (*entry)(void),
    void *stack_top
);

void context_switch(
    cpu_context_t *old_context,
    cpu_context_t *new_context
);

void context_switch_to_interrupt(
    cpu_context_t *old_context,
    void *interrupt_rsp
);

void context_resume_from_interrupt(cpu_context_t *context);

void context_start(task_t *task);

#endif
