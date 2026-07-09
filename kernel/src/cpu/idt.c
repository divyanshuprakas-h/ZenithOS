#include "idt.h"
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

struct  idtr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) ;

static struct idtr idtr;

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);


static void idt_set_gate(
    uint8_t vector,
    void *handler,
    uint16_t selector,
    uint8_t type_attributes
)
{
    uint64_t address = (uint64_t)handler;

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

    idt_set_gate(0,isr0,0x08,0x8E);
    idt_set_gate(1,isr1,0x08,0x8E);
    idt_set_gate(2,isr2,0x08,0x8E);
    idt_set_gate(3,isr3,0x08,0x8E);
    idt_set_gate(4,isr4,0x08,0x8E);
    idt_set_gate(5,isr5,0x08,0x8E);
    idt_set_gate(6,isr6,0x08,0x8E);
    idt_set_gate(7,isr7,0x08,0x8E);
    idt_set_gate(8,isr8,0x08,0x8E);
    idt_set_gate(9,isr9,0x08,0x8E);
    idt_set_gate(10,isr10,0x08,0x8E);
    idt_set_gate(11,isr11,0x08,0x8E);
    idt_set_gate(12,isr12,0x08,0x8E);
    idt_set_gate(13,isr13,0x08,0x8E);
    idt_set_gate(14,isr14,0x08,0x8E);
    idt_set_gate(15,isr15,0x08,0x8E);
    idt_set_gate(16,isr16,0x08,0x8E);
    idt_set_gate(17,isr17,0x08,0x8E);
    idt_set_gate(18,isr18,0x08,0x8E);
    idt_set_gate(19,isr19,0x08,0x8E);
    idt_set_gate(20,isr20,0x08,0x8E);
    idt_set_gate(21,isr21,0x08,0x8E);
    idt_set_gate(22,isr22,0x08,0x8E);
    idt_set_gate(23,isr23,0x08,0x8E);
    idt_set_gate(24,isr24,0x08,0x8E);
    idt_set_gate(25,isr25,0x08,0x8E);
    idt_set_gate(26,isr26,0x08,0x8E);
    idt_set_gate(27,isr27,0x08,0x8E);
    idt_set_gate(28,isr28,0x08,0x8E);
    idt_set_gate(29,isr29,0x08,0x8E);
    idt_set_gate(30,isr30,0x08,0x8E);
    idt_set_gate(31,isr31,0x08,0x8E);

    idt_load(&idtr);
    kprintf("IDT Loaded Successfully!\n");

    // kprintf("IDT Entry Size = %u\n",
    //     sizeof(struct idt_entry));

    // kprintf("IDTR Base = %p\n", (void *)idtr.base);
    // kprintf("IDTR Limit = %u\n" , idtr.limit); 
}