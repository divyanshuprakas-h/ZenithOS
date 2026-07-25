#include "task.h"
#include "config.h"

#include "../mm/heap.h"

static uint64_t next_task_id = 1;

task_t *task_create(void (*entry)(void))
{
    if (entry == NULL)
    {
        return NULL;
    }

    task_t *task = kmalloc(sizeof(task_t));

    if (task == NULL)
    {
        return NULL;
    }

    void *stack = kmalloc(KERNEL_STACK_SIZE);

    if (stack == NULL)
    {
        kfree(task);
        return NULL;
    }

    task->kernel_stack = stack;
    task->kernel_stack_size = KERNEL_STACK_SIZE;

    task->state = TASK_READY;
    task->next = NULL;

    task->id = next_task_id++;
    task->entry = entry;

    task->started = false;

    task->wake_tick = 0;

    void *stack_top = (uint8_t *)stack + KERNEL_STACK_SIZE;

    context_init(
        &task->context,
        entry,
        stack_top
    );

    return task;
}