#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"
#include "../mm/heap.h"
#include "../vmm/page_table.h"

void vmm_tests(void)
{
    terminal_write("  [TEST] Running VMM tests...\n");

    void *ptr = kmalloc(128);
    TEST_ASSERT(ptr != NULL);

    if (ptr != NULL)
    {
        uint64_t pa = page_table_translate((uint64_t)ptr);
        TEST_ASSERT(pa != 0);

        page_entry_t *entry = page_walk(page_table_get_pml4(), (uint64_t)ptr, false);
        TEST_ASSERT(entry != NULL);
        TEST_ASSERT(page_table_is_mapped((uint64_t)ptr));

        kfree(ptr);
    }
}
