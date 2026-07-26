#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../cpu/interrupts.h"
#include "../cpu/interrupt_context.h"

#include "task.h"

void scheduler_init(void);

void scheduler_add_task(task_t *task);

task_t *scheduler_get_next_task(void);

task_t *scheduler_schedule(void);

void scheduler_yield(void);

void idle_task(void);

void scheduler_block_current(void);

void scheduler_unblock(task_t *task);

void scheduler_sleep(uint64_t ticks);

uint64_t scheduler_get_ticks(void);

void scheduler_tick(void);

void scheduler_preempt(interrupt_context_t *context);

void scheduler_save_interrupt_context(void *interrupt_rsp);

void *scheduler_get_next_interrupt_rsp(void);

task_t *scheduler_peek_next_ready_task(void);

void scheduler_commit_pending_task(void);

#endif
