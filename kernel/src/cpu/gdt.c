#include "gdt.h"
#include "tss.h"

#include "../stdio/printf.h"
#include "../lib/memory.h"

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

static struct gdt_entry gdt[7];

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

static void gdt_set_tss_descriptor(
    int index,
    uint64_t base,
    uint32_t limit
)
{
    uint64_t descriptor_low = 0;
    uint64_t descriptor_high = 0;

    descriptor_low |= (limit & 0xFFFFULL);
    descriptor_low |= (base & 0xFFFFFFULL) << 16;
    descriptor_low |= (uint64_t)0x89 << 40;
    descriptor_low |= ((uint64_t)(limit >> 16) & 0xFULL) << 48;
    descriptor_low |= ((base >> 24) & 0xFFULL) << 56;

    descriptor_high = base >> 32;

    k_memcpy(&gdt[index], &descriptor_low, sizeof(uint64_t));
    k_memcpy(&gdt[index + 1], &descriptor_high, sizeof(uint64_t));

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

    gdt_set_entry(
        3,
        0,
        0xFFFFF,
        0xFA,
        0xA0
    );

    gdt_set_entry(
        4,
        0,
        0xFFFFF,
        0xF2,
        0xA0
    );

    gdt_set_tss_descriptor(
        5,
        (uint64_t)&kernel_tss,
        sizeof(kernel_tss) - 1
    );

    gdtr.limit = sizeof(gdt) - 1;

    kprintf("[GDT] TSS Base = %p Size = %u\n", (void *)&kernel_tss, (unsigned)sizeof(kernel_tss));

    gdtr.base = (uint64_t)&gdt;

    gdt_load(&gdtr);
    // kprintf("GDT Loaded Successfully!\n");

    // kprintf("GDTR Base  = %p\n", (void *)gdtr.base);
    // kprintf("GDTR Limit = %u\n", gdtr.limit);
}