#include "../process/process.h"
#include "../stdio/printf.h"

#include <stddef.h>


// void process_test(void)
// {
//     process_t *current = process_current();

//     if (current == NULL)
//     {
//         kprintf("ERROR: process_current() returned NULL!\n");
//     }
//     else
//     {
//         kprintf("Current PID: %llu\n", (unsigned long long)current->pid);
//         kprintf("Current Name: %s\n", current->name);
//     }
//     while (1)
//     {
//         scheduler_yield();
//     }
// }

void process_test(void)
{
    process_t *current = process_current();

    kprintf("Process %llu started\n", (unsigned long long)current->pid);

    process_exit(42);
}

void test_process_void(void)
{

    kprintf("==============PROCESS===================\n");

    process_t *p1 = process_create("Process 1", process_test);
    process_t *p2 = process_create("Process 2", process_test);
    process_t *p3 = process_create("Process 3", process_test);
    
    kprintf("P1 PID = %llu\n", (unsigned long long)p1->pid);
    kprintf("P2 PID = %llu\n", (unsigned long long)p2->pid);
    kprintf("P3 PID = %llu\n", (unsigned long long)p3->pid);

    kprintf("\n===== PROCESS LOOKUP TEST =====\n");

    process_t *lookup = process_lookup(2);

    if (lookup != NULL)
    {
        kprintf("Lookup Success!\n");
        kprintf("PID  : %llu\n", (unsigned long long)lookup->pid);
        kprintf("Name : %s\n", lookup->name);
    }
    else
    {
        kprintf("Lookup Failed\n");
    }

    kprintf("\n===== INVALID LOOKUP TEST =====\n");

    process_t *invalid = process_lookup(999);

    if (invalid == NULL)
    {
        kprintf("PASS: Invalid PID returned NULL\n");
    }
    else
    {
        kprintf("FAIL: Invalid PID should not exist!\n");
    }

    kprintf("=========================================\n");

}
