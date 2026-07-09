#include "gdt.h"
#include "../stdio/printf.h"

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

static struct gdt_entry gdt[3];

struct gdt_descriptor
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct  gdt_descriptor gdtr;

static void gdt_set_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity
)
{
    gdt[index].limit_low = limit & 0xFFFF;

    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].access = access;

    gdt[index].granularity = ((limit >> 16) & 0x0F);
    gdt[index].granularity |= granularity & 0xF0;

}


void gdt_init(void)

{
    gdt_set_entry(
        0,
        0,
        0,
        0,
        0
    );

    gdt_set_entry(
        1, 
        0,
        0xFFFFF,
        0x9A,
        0xA0
    );

    gdt_set_entry(
        2,
        0,
        0xFFFFF,
        0x92,
        0xA0
    );

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_load(&gdtr);
    // kprintf("GDT Loaded Successfully!\n");

    // kprintf("GDTR Base  = %p\n", (void *)gdtr.base);
    // kprintf("GDTR Limit = %u\n", gdtr.limit);
}