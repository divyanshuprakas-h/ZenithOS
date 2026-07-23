#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"
#include "../mm/pmm.h"

void pmm_tests(void)
{
    terminal_write("\n");
    terminal_write("========================================\n");
    terminal_write("            PMM TESTS\n");
    terminal_write("========================================\n");

    void *page1 = pmm_alloc_page();
    void *page2 = pmm_alloc_page();
    void *page3 = pmm_alloc_page();

    kprintf("Page1 : %p\n", page1);
    kprintf("Page2 : %p\n", page2);
    kprintf("Page3 : %p\n", page3);

    pmm_free_page(page2);

    terminal_write("Freed Page2\n");

    void *page4 = pmm_alloc_page();

    kprintf("Page4 : %p\n", page4);

    kprintf("Free Pages : %u\n",
            (unsigned)pmm_get_free_pages());

    kprintf("Used Pages : %u\n",
            (unsigned)pmm_get_used_pages());

    terminal_write("\nLock Test\n");

    kprintf("Before Lock : %u\n",
            (unsigned)pmm_get_free_pages());

    pmm_lock_page((void *)0x1000);

    kprintf("After Lock  : %u\n",
            (unsigned)pmm_get_free_pages());

    pmm_unlock_page((void *)0x1000);

    kprintf("After Unlock: %u\n",
            (unsigned)pmm_get_free_pages());

    terminal_write("\nPMM TESTS PASSED\n");
}