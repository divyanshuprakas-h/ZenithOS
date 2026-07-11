#ifndef ZENITH_MEMORY_MAP_H
#define ZENITH_MEMORY_MAP_H

#include <limine.h>

void memory_map_init(
    volatile struct limine_memmap_response *response
);

void memory_map_dump(void);

#endif

