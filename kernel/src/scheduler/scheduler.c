#include "scheduler.h"
#include "queue.h"

static task_t *ready_queue = NULL;
static task_t *current_task = NULL;

static task_t *idle = NULL;
static task_t *blocked_queue = NULL;
static task_t *sleep_queue = NULL;
static uint64_t task_count = 0;
static uint64_t scheduler_ticks = 0;

static void scheduler_sleep_queue_add(task_t *task);
static void scheduler_check_sleeping_tasks(void);

void scheduler_init(void)
{
    ready_queue = NULL;
    current_task = NULL;

    idle = task_create(idle_task);
    task_count = 0;

}

void scheduler_add_task(task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    task_queue_push(&ready_queue, task);

    task_count++;
}

task_t *scheduler_get_next_task(void)
{
    if (ready_queue == NULL)
    {
        return idle;
    }

    task_t *task = ready_queue;

    for (uint64_t i = 0; i < task_count; i++)
    {
        if (task->state == TASK_READY)
        {
            if (task == ready_queue && task->next != NULL)
            {
                ready_queue = ready_queue->next;

                task_t *tail = ready_queue;

                while (tail->next != NULL)
                {
                    tail = tail->next;
                }

                tail->next = task;
                task->next = NULL;
            }

            return task;
        }

        task = task->next;

        if (task == NULL)
        {
            task = ready_queue;
        }
    }

    return idle;
}


task_t *scheduler_schedule(void)
{
    task_t *next = scheduler_get_next_task();

    if (next == NULL)
    {
        return NULL;
    }

    current_task = next;
    current_task->state = TASK_RUNNING;

    return current_task;
}

void scheduler_yield(void)
{
    task_t *next = scheduler_get_next_task();

    if (next == NULL)
    {
        return;
    }

    if (current_task == next)
    {
        return;
    }

    task_t *previous = current_task;

    current_task = next;
    current_task->state = TASK_RUNNING;

    if (previous != NULL)
    {
        if (previous->state == TASK_RUNNING)
        {
            previous->state = TASK_READY;
        }

        if (!current_task->started)
        {
            current_task->started = true;
            context_start(current_task);
        }
        else{
            context_switch(
                &previous->context,
                &current_task->context
            );
        }
    }
    else
    {
        current_task->started = true;
        context_start(current_task);
    }
}

void scheduler_block_current(void)
{
    if(current_task == NULL)
    {
        return;
    }

    current_task->state = TASK_BLOCKED;

    task_queue_remove(&ready_queue, current_task);
    task_queue_push(&blocked_queue, current_task);

    scheduler_yield();
}

void scheduler_unblock(task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    if (task->state != TASK_BLOCKED)
    {
        return;
    }

    task_queue_remove(&blocked_queue, task);
    task->state = TASK_READY;
    task_queue_push(&blocked_queue, task);
}

static void scheduler_sleep_queue_add(task_t *task)
{
    task_queue_push(&sleep_queue, task);
}

static void scheduler_check_sleeping_tasks(void)
{
    task_t *task = sleep_queue;
    task_t *next;

    while (task != NULL)
    {
        next = task->next;

        if (scheduler_ticks >= task->wake_tick)
        {
            task_queue_remove(&sleep_queue, task);
            task->state = TASK_READY;
            task_queue_push(&ready_queue, task);
        }
        task = next;
    }
}

void scheduler_sleep(uint64_t ticks)
{
    if (current_task == NULL)
    {
        return;
    }

    current_task->wake_tick = scheduler_ticks + ticks;
    current_task->state = TASK_SLEEPING;

    task_queue_remove(&ready_queue, current_task);
    scheduler_sleep_queue_add(current_task);

    scheduler_yield();
}

uint64_t scheduler_get_ticks(void)
{
    return scheduler_ticks;
}

void scheduler_tick(void)
{
    scheduler_ticks++;

    scheduler_check_sleeping_tasks();
}

