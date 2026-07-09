#include "exceptions.h"

#include "../stdio/printf.h"

static void hcf(void)
{
    for(;;)
    {
        #if defined(__x86_64__)
            __asm__ volatile("hlt");
        #endif
    }
}

static const char *exception_names[32] = 
{
    "Divide Erroe",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid Tss",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Aligment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

void exception_dispatch(uint64_t interrupt_number)
{
    kprintf("\n");
    kprintf("================================\n");
    kprintf("CPU EXCEPTION\n");
    kprintf("================================\n");

    kprintf("Interrupt Number = %u\n", interrupt_number);

    if (interrupt_number < 32)
    {
        kprintf("%s\n", exception_names[interrupt_number]);
    }
    else
    {
        kprintf("Unknown Exception\n");
    }

    kprintf("\n System Halted \n");

    hcf();
}