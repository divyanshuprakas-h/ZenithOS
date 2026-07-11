#include "memory_map.h"
#include "../stdio/printf.h"

static volatile struct limine_memmap_response *memmap;

void memory_map_init(
    volatile struct limine_memmap_response *response
)
{
    memmap = response;
}

void memory_map_dump(void)
{
    kprintf("\n");
    kprintf("========== Memory Map ==========\n");

    for(uint64_t i = 0; i < memmap -> entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap->entries[i];

        kprintf(
            "[%u] Base = %p Length = %u KB Type = %u \n",
            (unsigned)i,
            (void *)entry->base,
            (unsigned)(entry->length / 1024),
            (unsigned)entry->type
        );
    }

    kprintf("====================================\n");

}