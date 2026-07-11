#include "idt.h"
#include "isr.h"

#include "../stdio/printf.h"

#include <stdint.h>

struct idt_entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

static struct idt_entry idt[256];

struct idtr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idtr idtr;

static void idt_set_gate(
    uint8_t vector,
    void (*handler)(void),
    uint16_t selector,
    uint8_t type_attributes
)
{
    uintptr_t address = (uintptr_t)handler;

    idt[vector].offset_low = address & 0xFFFF;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].type_attributes = type_attributes;
    idt[vector].offset_middle = (address >> 16) & 0xFFFF;
    idt[vector].offset_high = (address >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

void idt_init(void)
{
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint64_t)&idt;

    for (uint8_t vector = 0; vector < ISR_VECTOR_COUNT; ++vector)
    {
        idt_set_gate(vector, isr_table[vector], 0x08, 0x8E);
    }

    idt_load(&idtr);
    kprintf("IDT Loaded Successfully!\n");
}
