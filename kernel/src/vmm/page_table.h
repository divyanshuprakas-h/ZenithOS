#pragma once

#include <stdint.h>

#define PAGE_SIZE           4096ULL
#define PAGE_TABLE_ENTRIES  512

#define PAGE_PRESENT       (1ULL << 0)
#define PAGE_WRITABLE      (1ULL << 1)
#define PAGE_USER          (1ULL << 2)
#define PAGE_WRITE_THROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED      (1ULL << 5)
#define PAGE_DIRTY         (1ULL << 6)
#define PAGE_HUGE          (1ULL << 7)
#define PAGE_GLOBAL        (1ULL << 8)
#define PAGE_NO_EXECUTE    (1ULL << 63)

typedef uint64_t page_entry_t;

typedef struct page_table
{
    page_entry_t entries[PAGE_TABLE_ENTRIES];
}page_table_t;

page_table_t *page_table_get_pml4(void);

void page_table_set_hhdm_offset(uint64_t offset);

void page_table_init(void);
