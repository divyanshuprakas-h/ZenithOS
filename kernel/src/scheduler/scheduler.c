#include "scheduler.h"
#include "queue.h"
#include "config.h"

#include "../hal/pit.h"
#include "../lib/memory.h"
#include "../stdio/printf.h"
#include "../process/process.h"
#include "../cpu/tss.h"

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
static void scheduler_reset_resume_state(task_t *task);

static const char *scheduler_task_state_name(task_state_t state)
{
    switch (state)
    {
        case TASK_READY: return "READY";
        case TASK_RUNNING: return "RUNNING";
        case TASK_BLOCKED: return "BLOCKED";
        case TASK_SLEEPING: return "SLEEPING";
        case TASK_TERMINATED: return "TERMINATED";
        case TASK_IDLE: return "IDLE";
        default: return "UNKNOWN";
    }
}

static const char *scheduler_resume_mode_name(task_resume_mode_t mode)
{
    switch (mode)
    {
        case TASK_RESUME_CONTEXT: return "CONTEXT";
        case TASK_RESUME_INTERRUPT: return "INTERRUPT";
        default: return "UNKNOWN";
    }
}

static void scheduler_trace_task(const char *label, const task_t *task)
{
    if (task == NULL)
    {
        // kprintf("[TRACE] %s task=NULL\n", label);
        return;
    }

    // kprintf("[TRACE] %s task=%p id=%llu state=%s entry=%p stack=%p rsp=%p next=%p\n",
    //         label,
    //         task,
    //         (unsigned long long)task->id,
    //         scheduler_task_state_name(task->state),
    //         task->entry,
    //         task->kernel_stack,
    //         (void *)(uintptr_t)task->context.rsp,
    //         task->next);

    // kprintf("[TRACE] %s mode=%s interrupt_rsp=%p wake_tick=%llu\n",
    //         label,
    //         scheduler_resume_mode_name(task->resume_mode),
    //         task->interrupt_rsp,
    //         (unsigned long long)task->wake_tick);
}

static void scheduler_reset_resume_state(task_t *task)
{
    if (task == NULL)
    {
        return;
    }

    task->interrupt_rsp = NULL;
    task->resume_mode = TASK_RESUME_CONTEXT;
}

void scheduler_trace_context_switch_entry(cpu_context_t *old_context, cpu_context_t *new_context)
{
    // kprintf("[TRACE] context_switch entered old=%p new=%p new_rsp=%p\n",
    //         old_context,
    //         new_context,
    //         new_context != NULL ? (void *)(uintptr_t)new_context->rsp : NULL);
}


static task_t *scheduler_switch_to(task_t *next)
{
    scheduler_trace_task("scheduler_switch_to next", next);

    if (next == NULL)
    {
        // kprintf("[TRACE] scheduler_switch_to next=NULL\n");
        return NULL;
    }

    if (next->process != NULL){
        
        page_table_switch(next->process->page_table_physical);
        kprintf("[SCHED] Switching to PID=%llu CR3=%p\n", (unsigned long long)next->process->pid, (void *)next->process->page_table_physical);
        
    };

    if (next != NULL)
    {
        static uint64_t last_rsp0 = 0;

        uint64_t rsp0 = (uint64_t) next->kernel_stack + next->kernel_stack_size;

        if (rsp0 != last_rsp0)
        {
            last_rsp0 = rsp0;
            kprintf("[TSS] RSP0 = %p\n", (void *)rsp0);
        }

        tss_set_rsp0(rsp0);
        
    }

    current_task = next;
    preemption_ticks = 0;

    if (current_task != idle)
    {
        current_task->state = TASK_RUNNING;
    }

    scheduler_trace_task("scheduler_switch_to current", current_task);
    return current_task;
}

void scheduler_init(void)
{
    kprintf("[TRACE] scheduler_init entry\n");
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

    scheduler_trace_task("scheduler_init idle", idle);
}

void scheduler_add_task(task_t *task)
{
    scheduler_trace_task("scheduler_add_task before", task);
    scheduler_trace_task("scheduler_add_task ready_head_before", ready_queue);

    if (task == NULL)
    {
        // kprintf("[TRACE] scheduler_add_task task=NULL\n");
        return;
    }

    bool irq_was_enabled = interrupt_save();

    task->state = TASK_READY;
    task_queue_push(&ready_queue, task);

    interrupt_restore(irq_was_enabled);

    scheduler_trace_task("scheduler_add_task ready_head_after", ready_queue);
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
    scheduler_trace_task("scheduler_schedule ready_head", ready_queue);

    task_t *next = scheduler_get_next_task();

    if (next == NULL)
    {
        // kprintf("[TRACE] scheduler_schedule next=NULL\n");
        return NULL;
    }

    if (next != idle)
    {
        scheduler_ready_queue_pop();
    }

    scheduler_trace_task("scheduler_schedule selected", next);
    return scheduler_switch_to(next);
}

void scheduler_yield(void)
{
    // kprintf("[TRACE] scheduler_yield entry current=%p ready_head=%p\n",
    //         current_task,
    //         ready_queue);

    bool irq_was_enabled = interrupt_save();
    task_t *previous = current_task;

    scheduler_trace_task("scheduler_yield previous", previous);

    if (previous != NULL &&
        previous != idle &&
        previous->state == TASK_RUNNING)
    {
        previous->state = TASK_READY;
        scheduler_reset_resume_state(previous);
        task_queue_push(&ready_queue, previous);
        scheduler_trace_task("scheduler_yield requeued previous", previous);
    }

    task_t *next = scheduler_schedule();

    scheduler_trace_task("scheduler_yield next", next);

    if (next == NULL || next == previous)
    {
        // kprintf("[TRACE] scheduler_yield no switch next=%p previous=%p\n", next, previous);
        interrupt_restore(irq_was_enabled);
        return;
    }

    cpu_context_t *previous_context =
        previous != NULL ? &previous->context : &boot_context;

    // kprintf("[TRACE] scheduler_yield previous_context=%p next_context=%p\n",
    //         previous_context,
    //         &current_task->context);

    if (task_resume_uses_interrupt_frame(current_task))
    {
        void *next_interrupt_rsp = current_task->interrupt_rsp;
        interrupt_resume_frame_t *next_interrupt_frame =
            (interrupt_resume_frame_t *)next_interrupt_rsp;

        if (next_interrupt_rsp == NULL)
        {
            // kprintf("[TRACE] scheduler_yield interrupt resume missing rsp\n");
            interrupt_restore(irq_was_enabled);
            return;
        }

        // kprintf("[TRACE] scheduler_yield interrupt frame rip=%p cs=%p rflags=%p rsp=%p ss=%p\n",
        //         (void *)(uintptr_t)next_interrupt_frame->rip,
        //         (void *)(uintptr_t)next_interrupt_frame->cs,
        //         (void *)(uintptr_t)next_interrupt_frame->rflags,
        //         (void *)(uintptr_t)0,
        //         (void *)(uintptr_t)0);
        // kprintf("[TRACE] scheduler_yield context_switch_to_interrupt rsp=%p\n",
        //         next_interrupt_rsp);
        context_switch_to_interrupt(previous_context, next_interrupt_rsp);
    }
    else
    {
        // kprintf("[TRACE] scheduler_yield context_switch new_rsp=%p\n",
        //         (void *)(uintptr_t)current_task->context.rsp);
        context_switch(previous_context, &current_task->context);
    }

    interrupt_restore(irq_was_enabled);
    // kprintf("[TRACE] scheduler_yield exit current=%p\n", current_task);
}

task_t *scheduler_current_task(void)
{
    return current_task;
}

void scheduler_block_current_locked(void)
{
    if (current_task == NULL || current_task == idle)   
    {
        return;
    }

    current_task->state = TASK_BLOCKED;
    task_queue_push(&blocked_queue, current_task);

    scheduler_yield();
}

void scheduler_block_current(void)
{
    bool irq_was_enabled = interrupt_save();

    scheduler_block_current_locked();

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
    scheduler_reset_resume_state(task);
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
            scheduler_reset_resume_state(task);
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
        // kprintf("[TRACE] scheduler_ready_queue_pop empty\n");
        return NULL;
    }

    ready_queue = task->next;
    task->next = NULL;

    scheduler_trace_task("scheduler_ready_queue_pop popped", task);
    scheduler_trace_task("scheduler_ready_queue_pop new_head", ready_queue);

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

    // kprintf("[TRACE] scheduler_save_interrupt_context task=%p rsp=%p\n",
    //         current_task,
    //         interrupt_rsp);
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


