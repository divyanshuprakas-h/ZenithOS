#include "page_table.h"
#include "paging.h"

#include "../mm/pmm.h"
#include "../stdio/printf.h"

void paging_init(void)
{
    kprintf("Free Pages Before : %u\n", (unsigned)pmm_get_free_pages());
    page_table_init();
    kprintf("Free Pages After  : %u\n", (unsigned)pmm_get_free_pages());

    kprintf("\n===== Paging =====\n");
    kprintf("Page Size          :%u\n", (unsigned)PAGE_SIZE);
    kprintf("Table Entries      :%u\n", (unsigned)PAGE_TABLE_ENTRIES);
    kprintf("Table Size         : %u\n", (unsigned)sizeof(page_table_t));

    kprintf("Activating page tables...\n");
    page_table_activate();
    kprintf("Paging Active!\n");
    
}
