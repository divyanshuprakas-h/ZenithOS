#include "scheduler.h"

void idle_task(void)
{
    while (1)
    {
        __asm__ volatile("hlt");
    }
    
}