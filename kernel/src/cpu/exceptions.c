#include "exceptions.h"
#include "exceptions/page_fault.h"
#include "exceptions/panic.h"
#include "../process/process.h"

#include "../stdio/printf.h"

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

void exception_dispatch(interrupt_context_t *context)
{
    switch (context->interrupt_number)
    {
        case EXCEPTION_PAGE_FAULT:
            page_fault_handler(context);
            break;

        default:
            if ((context->frame.cs & 3) == 3)
            {
                process_t *proc = process_current();
                const char *ex_name = context->interrupt_number < 32 ?
                                      exception_names[context->interrupt_number] : "Unknown";
                kprintf("[USER EXCEPTION] PID %llu vector %u (%s) at RIP=%p\n",
                        proc ? (unsigned long long)proc->pid : 0,
                        (unsigned int)context->interrupt_number, ex_name, (void *)context->frame.rip);
                process_exit(-1);
                return;
            }
            kernel_panic(
                exception_names[context->interrupt_number],
                context
            );
            break;
    }
}