#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"
#include "../mm/heap.h"

void heap_tests(void)
{
    terminal_write("  [TEST] Running Heap tests...\n");

    void *a = kmalloc(64);
    TEST_ASSERT(a != NULL);

    void *b = kmalloc(128);
    TEST_ASSERT(b != NULL);
    TEST_ASSERT(a != b);

    kfree(a);
    kfree(b);

    uint32_t *arr = (uint32_t *)kcalloc(16, sizeof(uint32_t));
    TEST_ASSERT(arr != NULL);
    if (arr != NULL)
    {
        bool all_zero = true;
        for (int i = 0; i < 16; i++)
        {
            if (arr[i] != 0)
                all_zero = false;
        }
        TEST_ASSERT(all_zero);
        kfree(arr);
    }

    char *str = kmalloc(16);
    TEST_ASSERT(str != NULL);
    if (str != NULL)
    {
        str[0] = 'H';
        str[1] = 'i';
        str[2] = '\0';
        char *restr = krealloc(str, 64);
        TEST_ASSERT(restr != NULL);
        if (restr != NULL)
        {
            TEST_ASSERT(restr[0] == 'H' && restr[1] == 'i');
            kfree(restr);
        }
        else
        {
            kfree(str);
        }
    }
}
