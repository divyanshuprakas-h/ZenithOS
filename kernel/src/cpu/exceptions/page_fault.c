#include "page_fault.h"
#include "panic.h"

#include "../../stdio/printf.h"
#include "../../vmm/demand_paging.h"

static uint64_t read_cr2(void)
{
    uint64_t value;

    __asm__ volatile(
        "mov %%cr2, %0"
        : "=r"(value)
    );

    return value;
}

void page_fault_handler(interrupt_context_t *context)
{
    (void)context;

    uint64_t fault_address = read_cr2();
    uint64_t error = context->error_code;

    bool present        = error & (1<<0);
    bool write          = error & (1<<1);
    bool user           = error & (1<<2);
    bool reserved       = error & (1<<3);
    bool instruction    = error & (1<<4);

    kprintf("Page Fault!\n");
    kprintf("Fault Address = %p\n", (void *)fault_address);

    kprintf("=========================\n");
    kprintf("Present        :%s\n", present ? "Yes":"No");
    kprintf("Write          :%s\n", write ? "Yes":"No");
    kprintf("User Mode      :%s\n", user ? "Yes":"No");
    kprintf("Reserved Bit   :%s\n", reserved ? "Yes":"No");
    kprintf("Instruction    :%s\n", instruction ? "Yes":"No");
    kprintf("=========================\n");

    if (vmm_handle_page_fault(fault_address, error))
    {
        return;
    }

    kernel_panic("PAGE FAULT", context);

    for(;;)
    {
        __asm__ volatile("hlt");
    }

}