#include "tests.h"

#include "../terminal/terminal.h"
#include "../stdio/printf.h"
#include "../mm/pmm.h"

void pmm_tests(void)
{
    terminal_write("  [TEST] Running PMM tests...\n");

    void *page1 = pmm_alloc_page();
    TEST_ASSERT(page1 != NULL);

    void *page2 = pmm_alloc_page();
    TEST_ASSERT(page2 != NULL);
    TEST_ASSERT(page1 != page2);

    void *page3 = pmm_alloc_page();
    TEST_ASSERT(page3 != NULL);

    uint64_t free_before = pmm_get_free_pages();
    pmm_free_page(page2);
    uint64_t free_after = pmm_get_free_pages();
    TEST_ASSERT(free_after == free_before + 1);

    void *page4 = pmm_alloc_page();
    TEST_ASSERT(page4 != NULL);

    pmm_free_page(page1);
    pmm_free_page(page3);
    pmm_free_page(page4);
}
