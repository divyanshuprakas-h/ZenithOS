#include "task.h"
#include "config.h"

#include "../mm/heap.h"
#include "../stdio/printf.h"

#include <stddef.h>

static uint64_t next_task_id = 1;

_Static_assert(offsetof(task_t, context) == 8, "task_t.context offset must match context.asm");
_Static_assert(offsetof(task_t, entry) == 72, "task_t.entry offset must match context.asm");
_Static_assert(offsetof(task_t, kernel_stack) == 80, "task_t.kernel_stack offset must match context.asm");
_Static_assert(offsetof(task_t, next) == 104, "task_t.next offset must match context.asm");

task_t *task_create(void (*entry)(void))
{
    kprintf("[TRACE] task_create entry=%p\n", entry);

    if (entry == NULL)
    {
        kprintf("[TRACE] task_create null entry\n");
        return NULL;
    }

    task_t *task = kmalloc(sizeof(task_t));
    kprintf("[TRACE] task_create task=%p\n", task);

    if (task == NULL)
    {
        kprintf("[TRACE] task_create failed allocating task\n");
        return NULL;
    }

    void *stack = kmalloc(KERNEL_STACK_SIZE);
    kprintf("[TRACE] task_create stack=%p size=%u\n", stack, (unsigned)KERNEL_STACK_SIZE);

    if (stack == NULL)
    {
        kfree(task);
        kprintf("[TRACE] task_create failed allocating stack\n");
        return NULL;
    }

    task->kernel_stack = stack;
    task->kernel_stack_size = KERNEL_STACK_SIZE;

    task->state = TASK_READY;
    task->next = NULL;

    task->id = next_task_id++;
    task->entry = entry;

    task->wake_tick = 0;

    task->interrupt_rsp = NULL;
    task->resume_mode = TASK_RESUME_CONTEXT;

    void *stack_top = (uint8_t *)stack + KERNEL_STACK_SIZE;

    context_init(
        &task->context,
        entry,
        stack_top
    );

    kprintf("[TRACE] task_create kernel_stack=%p stack_top=%p context_rsp=%p\n",
            task->kernel_stack,
            stack_top,
            (void *)(uintptr_t)task->context.rsp);

    return task;
}
