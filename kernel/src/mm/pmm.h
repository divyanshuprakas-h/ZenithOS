#ifndef ZENITH_PMM_H
#define ZENITH_PMM_H

#include <stdint.h>
#include <limine.h>

void pmm_init(
    volatile struct limine_memmap_response *response,
    volatile struct limine_executable_address_response *executable_response
);

void *pmm_alloc_page(void);

void pmm_free_page(void *address);

void pmm_lock_page(void *address);

void pmm_unlock_page(void *address);

void pmm_reserve_region(uint64_t base, uint64_t length);

void pmm_unreserve_region(uint64_t base, uint64_t length);

void pmm_reserve_page(void *address);

void pmm_unreserve_page(void *address);

void pmm_unlock_pages(void *address, uint64_t count);

void pmm_lock_pages(void *address, uint64_t count);

uint64_t pmm_get_free_pages(void);

uint64_t pmm_get_used_pages(void);

uint64_t pmm_get_total_pages(void);

#endif
