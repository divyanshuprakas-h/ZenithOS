#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"

#include "../mm/heap.h"

#include "../vmm/page_table.h"

void demand_paging_test(void);

void vmm_tests(void)
{
    terminal_write("\n------------ VMM TESTS ------------\n");

    void *ptr = kmalloc(128);

    kprintf("Virtual Address : %p\n", ptr);

    uint64_t pa = page_table_translate((uint64_t)ptr);

    kprintf("Physical Address : %p\n", (void *)pa);

    page_entry_t *entry =
        page_walk(page_table_get_pml4(),
                  (uint64_t)ptr,
                  false);

    kprintf("Entry Pointer : %p\n", entry);

    if (entry)
        kprintf("Entry Value : %016llx\n", *entry);

    kprintf("Mapped : %s\n",
        page_table_is_mapped((uint64_t)ptr)
            ? "YES"
            : "NO");

    page_table_dump((uint64_t)ptr);
    demand_paging_test();

    terminal_write("VMM TESTS PASSED\n");
}

void demand_paging_test(void)
{
    kprintf("\n");
    kprintf("===== Demand Paging Test =====\n");

    volatile uint64_t *ptr = (volatile uint64_t *)0xFFFF900000000000ULL;

    kprintf("Writing to %p...\n", ptr);

    *ptr = 0xDEADBEEFCAFEBABEULL;

    kprintf("Write completed.\n");

    uint64_t value = *ptr;

    kprintf("Read Value : %llx\n", value);

    if (value == 0xDEADBEEFCAFEBABEULL)
    {
        kprintf("Demand Paging Test PASSED\n");
    }
    else
    {
        kprintf("Demand Paging Test FAILED\n");
    }
}


