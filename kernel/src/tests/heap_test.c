#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"
#include "../mm/heap.h"

void heap_tests(void)
{
    terminal_write("\n------------ HEAP TESTS ------------\n");

    void *a = kmalloc(64);
    void *b = kmalloc(128);
    void *c = kmalloc(256);

    kprintf("Allocated A : %p\n", a);
    kprintf("Allocated B : %p\n", b);
    kprintf("Allocated C : %p\n", c);

    kfree(a);
    terminal_write("Freed A\n");

    kfree(b);
    terminal_write("Freed B\n");

    kfree(c);
    terminal_write("Freed C\n");

    void *d = kmalloc(128);

    kprintf("Reallocated D : %p\n", d);

    uint32_t *arr = (uint32_t *)kcalloc(16, sizeof(uint32_t));

    terminal_write("kcalloc:\n");

    for (int i = 0; i < 16; i++)
        kprintf("%u ", arr[i]);

    kprintf("\n");

    arr[5] = 1234;

    terminal_write("Modified:\n");

    for (int i = 0; i < 16; i++)
        kprintf("%u ", arr[i]);

    kprintf("\n");

    char *str = kmalloc(16);

    str[0] = 'H';
    str[1] = 'i';
    str[2] = '\0';

    kprintf("%s\n", str);

    str = krealloc(str,64);

    kprintf("%s\n",str);

    terminal_write("HEAP TESTS PASSED\n");
}