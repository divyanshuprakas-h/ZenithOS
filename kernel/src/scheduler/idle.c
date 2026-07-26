#include "scheduler.h"

void idle_task(void)
{
    while (1)
    {
        scheduler_yield();
        __asm__ volatile("hlt");
    }
}
