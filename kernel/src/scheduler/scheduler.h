#ifndef SCHEDULER_H
#define SCHEDULER_H

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

#endif

