#include "../terminal/terminal.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/task.h"
#include "../stdio/printf.h"

static task_t *sleep_test_task_a = NULL;
static task_t *sleep_test_task_b = NULL;

static void task_a(void)
{
    terminal_write("Test: A started\n");

    while (1)
    {
        terminal_putchar('A');
        terminal_render();

        for (volatile uint64_t i = 0; i < 1000000; i++)
        {

        }
    }
}

static void task_b(void)
{
    terminal_write("Task: B Started\n");

    while (1)
    {
        terminal_putchar('B');
        terminal_render();
    }
}

void scheduler_run_sleep_test(void)
{
    terminal_write("Starting scheduler sleep test\n");

    sleep_test_task_a = task_create(task_a);
    sleep_test_task_b = task_create(task_b);

    if (sleep_test_task_a == NULL || sleep_test_task_b == NULL)
    {
        terminal_write("Scheduler sleep test setup failed\n");
        return;
    }

    scheduler_add_task(sleep_test_task_a);
    scheduler_add_task(sleep_test_task_b);
    scheduler_yield();
}
