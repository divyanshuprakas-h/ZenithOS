#include "scheduler.h"
#include "queue.h"
#include "config.h"

#include "../hal/pit.h"
#include "../lib/memory.h"

#define KERNEL_CODE_SELECTOR 0x08ULL
#define KERNEL_TRAMPOLINE_RFLAGS 0x02ULL

static task_t *ready_queue = NULL;
static task_t *current_task = NULL;

static task_t *idle = NULL;
static task_t *blocked_queue = NULL;
static task_t *sleep_queue = NULL;

static task_t *pending_task = NULL;

static uint64_t preemption_ticks = 0;
static bool preemption_requested = false;

static cpu_context_t boot_context;

typedef struct interrupt_resume_frame
{
    registers_t regs;
    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} interrupt_resume_frame_t;

static task_t *scheduler_ready_queue_pop(void);
static void scheduler_sleep_queue_add(task_t *task);
static void scheduler_check_sleeping_tasks(void);
static void *scheduler_build_context_interrupt_rsp(task_t *task);


static task_t *scheduler_switch_to(task_t *next)
{
    if (next == NULL)
    {
        return NULL;
    }

    current_task = next;
    preemption_ticks = 0;

    if (current_task != idle)
    {
        current_task->state = TASK_RUNNING;
    }
    return current_task;
}

void scheduler_init(void)
{
    ready_queue = NULL;
    current_task = NULL;
    blocked_queue = NULL;
    sleep_queue = NULL;
    pending_task = NULL;
    preemption_ticks = 0;
    preemption_requested = false;

    idle = task_create(idle_task); 

    if (idle != NULL)
    {
        idle->state = TASK_IDLE;
        idle->next = NULL;
    }
}

void scheduler_add_task(task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    bool irq_was_enabled = interrupt_save();

    task->state = TASK_READY;
    task_queue_push(&ready_queue, task);

    interrupt_restore(irq_was_enabled);
}

task_t *scheduler_get_next_task(void)
{
    if (ready_queue != NULL)
    {
        return ready_queue;
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

    if (next != idle)
    {
        next = scheduler_ready_queue_pop();
    }

    return scheduler_switch_to(next);
}

void scheduler_yield(void)
{
    bool irq_was_enabled = interrupt_save();
    task_t *previous = current_task;

    if (previous != NULL &&
        previous != idle &&
        previous->state == TASK_RUNNING)
    {
        previous->state = TASK_READY;
        previous->interrupt_rsp = NULL;
        previous->resume_mode = TASK_RESUME_CONTEXT;
        task_queue_push(&ready_queue, previous);
    }

    task_t *next = scheduler_schedule();

    if (next == NULL || next == previous)
    {
        interrupt_restore(irq_was_enabled);
        return;
    }

    cpu_context_t *previous_context =
        previous != NULL ? &previous->context : &boot_context;

    if (task_resume_uses_interrupt_frame(current_task))
    {
        void *next_interrupt_rsp = current_task->interrupt_rsp;

        if (next_interrupt_rsp == NULL)
        {
            interrupt_restore(irq_was_enabled);
            return;
        }

        context_switch_to_interrupt(previous_context, next_interrupt_rsp);
    }
    else
    {
        context_switch(previous_context, &current_task->context);
    }

    interrupt_restore(irq_was_enabled);
}

void scheduler_block_current(void)
{
    if (current_task == NULL || current_task == idle)
    {
        return;
    }

    bool irq_was_enabled = interrupt_save();

    current_task->state = TASK_BLOCKED;

    task_queue_push(&blocked_queue, current_task);

    scheduler_yield();

    interrupt_restore(irq_was_enabled);
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

    bool irq_was_enabled = interrupt_save();

    task_queue_remove(&blocked_queue, task);
    task->state = TASK_READY;
    task_queue_push(&ready_queue, task);

    interrupt_restore(irq_was_enabled);
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

        if (pit_ticks() >= task->wake_tick)
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
    if (current_task == NULL || current_task == idle)
    {
        return;
    }

    bool irq_was_enabled = interrupt_save();

    current_task->wake_tick = pit_ticks() + ticks;
    current_task->state = TASK_SLEEPING;

    scheduler_sleep_queue_add(current_task);

    scheduler_yield();

    interrupt_restore(irq_was_enabled);
}

void scheduler_tick(void)
{
    scheduler_check_sleeping_tasks();
}

uint64_t scheduler_get_ticks(void)
{
    return pit_ticks();
}

static task_t *scheduler_ready_queue_pop(void)
{
    task_t *task = ready_queue;

    if (task == NULL)
    {
        return NULL;
    }

    ready_queue = task->next;
    task->next = NULL;

    return task;
}

static void *scheduler_build_context_interrupt_rsp(task_t *task)
{
    if (task == NULL)
    {
        return NULL;
    }

    if (task->context.rsp == 0)
    {
        return NULL;
    }

    interrupt_resume_frame_t *frame =
        (interrupt_resume_frame_t *)((uint8_t *)(uintptr_t)task->context.rsp -
                                     sizeof(interrupt_resume_frame_t));

    k_memset(frame, 0, sizeof(*frame));

    frame->regs.rdi = (uint64_t)(uintptr_t)&task->context;
    frame->rip = (uint64_t)(uintptr_t)context_resume_from_interrupt;
    frame->cs = KERNEL_CODE_SELECTOR;
    frame->rflags = KERNEL_TRAMPOLINE_RFLAGS;

    return frame;
}

void scheduler_preempt(interrupt_context_t *context)
{

    if (context == NULL)
    {
        return;
    }

    if (current_task == NULL)
    {
        return;
    }

    if (current_task == idle)
    {
        return;
    }

    preemption_ticks++;

    if (preemption_ticks < SCHEDULER_TIME_SLICE)
    {
        return;
    }

    preemption_ticks = 0;

    if (ready_queue == NULL)
    {
        return;
    }

    preemption_requested = true;
}

void scheduler_save_interrupt_context(void *interrupt_rsp)
{
    if (current_task == NULL)
    {
        return;
    }

    if (current_task == idle)
    {
        return;
    }

    current_task->interrupt_rsp = interrupt_rsp;
    current_task->resume_mode = TASK_RESUME_INTERRUPT;
}

void *scheduler_get_next_interrupt_rsp(void)
{
    if (!preemption_requested)
    {
        return NULL;
    }

    task_t *next = scheduler_get_next_task();

    if (next == NULL || next == idle)
    {
        preemption_requested = false;
        return NULL;
    }

    void *next_interrupt_rsp =
        task_resume_uses_interrupt_frame(next)
            ? next->interrupt_rsp
            : scheduler_build_context_interrupt_rsp(next);

    if (next_interrupt_rsp == NULL)
    {
        preemption_requested = false;
        return NULL;
    }

    scheduler_ready_queue_pop();

    task_t *previous = current_task;

    if (previous != NULL &&
        previous != idle &&
        previous->state == TASK_RUNNING)
    {
        previous->state = TASK_READY;
        task_queue_push(&ready_queue, previous);
    }

    pending_task = next;
    preemption_requested = false;

    return next_interrupt_rsp;
}

task_t *scheduler_peek_next_ready_task(void)
{
    if (ready_queue == NULL)
    {
        return idle;
    }

    return ready_queue;
}

void scheduler_commit_pending_task(void)
{
    if (pending_task == NULL)
    {
        return;
    }

    current_task = pending_task;
    preemption_ticks = 0;
    
    if (current_task != idle)
    {
        current_task->state = TASK_RUNNING;
    }

    pending_task = NULL;
}
