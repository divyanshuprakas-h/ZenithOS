#include "exceptions.h"

#include "../stdio/printf.h"

static void halt_forever(void)
{
    for (;;)
    {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#endif
    }
}

static const char *const exception_names[32] =
{
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

static void print_register(const char *name, uint64_t value)
{
    kprintf("%s = %p\n", name, (void *)value);
}

void exception_dispatch(interrupt_context_t *context)
{
    uint64_t interrupt_number = context->interrupt_number;
    const char *name = "Unknown Exception";

    if (interrupt_number < 32)
    {
        name = exception_names[interrupt_number];
    }

    kprintf("\n");
    kprintf("================================\n");
    kprintf("CPU EXCEPTION\n");
    kprintf("================================\n");
    kprintf("Vector      = %u\n", (unsigned int)interrupt_number);
    kprintf("Name        = %s\n", name);
    kprintf("Error Code  = %p\n", (void *)(uintptr_t)context->error_code);
    kprintf("RIP         = %p\n", (void *)(uintptr_t)context->frame.rip);
    kprintf("CS          = %p\n", (void *)(uintptr_t)context->frame.cs);
    kprintf("RFLAGS      = %p\n", (void *)(uintptr_t)context->frame.rflags);
    kprintf("\n");
    print_register("RAX", context->regs.rax);
    print_register("RBX", context->regs.rbx);
    print_register("RCX", context->regs.rcx);
    print_register("RDX", context->regs.rdx);
    print_register("RBP", context->regs.rbp);
    print_register("RSI", context->regs.rsi);
    print_register("RDI", context->regs.rdi);
    print_register("R8", context->regs.r8);
    print_register("R9", context->regs.r9);
    print_register("R10", context->regs.r10);
    print_register("R11", context->regs.r11);
    print_register("R12", context->regs.r12);
    print_register("R13", context->regs.r13);
    print_register("R14", context->regs.r14);
    print_register("R15", context->regs.r15);
    kprintf("\nSystem halted.\n");

    halt_forever();
}
