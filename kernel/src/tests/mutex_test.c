#include "../sync/mutex.h"
#include "../stdio/printf.h"
#include "../scheduler/scheduler.h"

void test_mutex_init(void)
{
    mutex_t mutex;

    mutex_init(&mutex);

    if (!mutex.locked &&
        mutex.owner == NULL &&
        mutex.wait_queue == NULL)

    {
        kprintf("[PASS] mutex init\n");
    }

    else
    {
        kprintf("[FAIL] mutex init\n");
    }
}

void test_mutex_lock(void)
{
    task_t *current_task = scheduler_current_task();

    if (current_task == NULL)
    {
        kprintf("[SKIP] mutex lock requires a running task\n");
        return;
    }

    mutex_t mutex;

    mutex_init(&mutex);

    mutex_lock(&mutex);

    if (mutex.locked &&
        mutex.owner == current_task)
    {
        kprintf("[PASS] mutex lock\n");
    }
    else
    {
        kprintf("[FAIL] mutex llock\n");
    }

}
