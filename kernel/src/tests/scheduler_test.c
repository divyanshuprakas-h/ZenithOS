#include "../terminal/terminal.h"
#include "../scheduler/scheduler.h"
#include "../scheduler/task.h"

void scheduler_test_task(void)
{
    while (1)
    {
        terminal_write("Scheduler Task Running\n");

        for (volatile uint64_t i = 0; i < 50000000; i++)
        {

        }
    }
}

void task_a(void)
{
    terminal_write("A Started\n");
    scheduler_block_current();

    while(1)
    {
        terminal_write("A Started\n");

        scheduler_yield();
    }
}

void task_b(void)
{
    while(1)
    {
        terminal_write("B\n");

        scheduler_yield();
    }
}