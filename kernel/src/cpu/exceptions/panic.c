#include "panic.h"

#include "../../stdio/printf.h"

void kernel_panic(const char *reason, interrupt_context_t *context)
{
    kprintf("\n");
    kprintf("========================================\n");
    kprintf("              KERNEL PANIC\n");
    kprintf("========================================\n");

    kprintf("\nException : %s\n", reason);
    
    kprintf("Interrupt : %llu\n", context->interrupt_number);
    kprintf("Error Code: 0x%016llx\n", context->error_code);

    kprintf("\nCPU State\n");
    kprintf("----------------------------------------\n");

    kprintf("RIP    : %p\n", (void *)context->frame.rip);
    kprintf("CS     : 0x%llx\n", context->frame.cs);
    kprintf("RFLAGS : 0x%llx\n", context->frame.rflags);
    kprintf("RSP    : %p\n", (void *)context->frame.rsp);
    kprintf("SS     : 0x%llx\n", context->frame.ss);

    kprintf("\nGeneral Registers\n");
    kprintf("----------------------------------------\n");

    kprintf("RAX : %016llx\n", context->regs.rax);
    kprintf("RBX : %016llx\n", context->regs.rbx);
    kprintf("RCX : %016llx\n", context->regs.rcx);
    kprintf("RDX : %016llx\n", context->regs.rdx);

    kprintf("RSI : %016llx\n", context->regs.rsi);
    kprintf("RDI : %016llx\n", context->regs.rdi);
    kprintf("RBP : %016llx\n", context->regs.rbp);

    kprintf("R8  : %016llx\n", context->regs.r8);
    kprintf("R9  : %016llx\n", context->regs.r9);
    kprintf("R10 : %016llx\n", context->regs.r10);
    kprintf("R11 : %016llx\n", context->regs.r11);
    kprintf("R12 : %016llx\n", context->regs.r12);
    kprintf("R13 : %016llx\n", context->regs.r13);
    kprintf("R14 : %016llx\n", context->regs.r14);
    kprintf("R15 : %016llx\n", context->regs.r15);

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}