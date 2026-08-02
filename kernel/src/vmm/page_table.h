#pragma once

#include <stdint.h>
#include <stdbool.h>

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


#define PML4_INDEX(addr) (((uint64_t)(addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((uint64_t)(addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((uint64_t)(addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((uint64_t)(addr) >> 12) & 0x1FF)
#define PAGE_OFFSET(addr) ((uint64_t)(addr) & 0xFFF)

typedef uint64_t page_entry_t;

typedef struct page_table
{
    page_entry_t entries[PAGE_TABLE_ENTRIES];
}page_table_t;

enum
{
    PAGE_TABLE_ADDRESS_MASK = 0x000FFFFFFFFFF000ULL,
    PAGE_2MIB_SIZE = 1ULL << 21,
    PAGE_1GIB_SIZE = 1ULL << 30,
};

page_table_t *page_table_get_pml4(void);

page_table_t *page_table_create(uint64_t *physical_out);

static inline bool page_is_present(page_entry_t entry)
{
    return (entry & PAGE_PRESENT) != 0;
}

static inline bool page_is_writable(page_entry_t entry)
{
    return (entry & PAGE_WRITABLE) != 0;
}

static inline bool page_is_user(page_entry_t entry)
{
    return (entry & PAGE_USER) != 0;
}

static inline bool page_is_executable(page_entry_t entry)
{
    return (entry & PAGE_NO_EXECUTE) == 0;
}

void page_table_set_hhdm_offset(uint64_t offset);

void page_table_init(void);

void *page_table_physical_to_virtual(uint64_t physical_address);

bool page_table_map(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);

bool page_table_map_page(uint64_t virtual_address, uint64_t physical_address, bool writable);

bool page_table_unmap(uint64_t virtual_address);

uint64_t page_table_translate(uint64_t virtual_address);

void page_table_activate(void);

page_entry_t *page_walk(
    page_table_t *pml4,
    uint64_t virtual_address,
    bool create
);

bool page_table_map_user(
    page_table_t *pml4,
    uint64_t virtual_address,
    uint64_t physical_address,
    bool writable
);

bool page_table_is_mapped(uint64_t virtual_address);

void page_table_dump(uint64_t virtual_address);

void page_table_switch(uint64_t physical_pml4);

