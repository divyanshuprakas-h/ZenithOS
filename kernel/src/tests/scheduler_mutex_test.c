#include "scheduler_mutex_test.h"

#include "../scheduler/scheduler.h"
#include "../scheduler/task.h"
#include "../stdio/printf.h"
#include "../sync/mutex.h"

static mutex_t test_mutex;

static void yield_forever(void)
{
    while (1)
    {
        scheduler_yield();
    }
}

static void mutex_task_a(void)
{
    kprintf("Task A:\n");

    mutex_lock(&test_mutex);
    kprintf("A: acquired\n");

    scheduler_sleep(20);

    kprintf("A: unlocking\n");
    mutex_unlock(&test_mutex);

    yield_forever();
}

static void mutex_task_b(void)
{
    kprintf("Task B:\n");

    mutex_lock(&test_mutex);
    kprintf("B: acquired\n");

    mutex_unlock(&test_mutex);

    yield_forever();
}

void scheduler_mutex_test(void)
{
    kprintf("[TRACE] scheduler_mutex_test entry\n");
    mutex_init(&test_mutex);
    kprintf("[TRACE] mutex initialized\n");

    kprintf("Create Task A\n");
    task_t *task_a = task_create(mutex_task_a);
    kprintf("[TRACE] task_a=%p stack=%p rsp=%p\n",
            task_a,
            task_a != NULL ? task_a->kernel_stack : NULL,
            task_a != NULL ? (void *)(uintptr_t)task_a->context.rsp : NULL);

    if (task_a == NULL)
    {
        kprintf("scheduler_mutex_test setup failed\n");
        return;
    }

    kprintf("Create Task B\n");
    task_t *task_b = task_create(mutex_task_b);
    kprintf("[TRACE] task_b=%p stack=%p rsp=%p\n",
            task_b,
            task_b != NULL ? task_b->kernel_stack : NULL,
            task_b != NULL ? (void *)(uintptr_t)task_b->context.rsp : NULL);

    if (task_b == NULL)
    {
        kprintf("scheduler_mutex_test setup failed\n");
        return;
    }

    scheduler_add_task(task_a);
    kprintf("[TRACE] ready head after task_a=%p\n", scheduler_peek_next_ready_task());
    scheduler_add_task(task_b);
    kprintf("[TRACE] ready head after task_b=%p\n", scheduler_peek_next_ready_task());

    kprintf("[TRACE] current_task before start=%p\n", scheduler_current_task());
    kprintf("Scheduler starts\n");
    scheduler_yield();
    kprintf("[TRACE] scheduler_yield returned to scheduler_mutex_test\n");
}
