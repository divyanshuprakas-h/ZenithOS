#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include "framebuffer/framebuffer.h"
#include "terminal/terminal.h"
#include "mm/heap.h"
#include "stdio/printf.h"
#include "lib/convert.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/exceptions.h"

// Limine Base Revision

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// Framebuffer Request

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Limine Start / End Markers

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

// Halt CPU

static void hcf(void)
{
    for (;;)
    {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#elif defined(__aarch64__) || defined(__riscv)
        __asm__ volatile("wfi");
#endif
    }
}

// Kernel Entry

static uint8_t kernel_heap[1024 * 1024];

void kmain(void)
{
    // Check Limine revision
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision))
    {
        hcf();
    }

    // Check framebuffer
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1)
    {
        hcf();
    }

    // Get framebuffer
    struct limine_framebuffer *framebuffer =
        framebuffer_request.response->framebuffers[0];

    framebuffer_init(
        framebuffer->address,
        framebuffer->width,
        framebuffer->height,
        framebuffer->pitch
    );

    heap_init(kernel_heap, sizeof(kernel_heap));

    framebuffer_clear(0x000000);

    terminal_init();

    terminal_write("====================================\n");
    terminal_write("          ZenithOS Kernel\n");
    terminal_write("====================================\n\n");

    terminal_write("Framebuffer : OK\n");
    terminal_write("Graphics    : OK\n");
    terminal_write("Font        : OK\n");
    terminal_write("Terminal    : OK\n\n");

    terminal_write("Before GDT\n");

    gdt_init();

    terminal_write("After GDT\n");

    idt_init();

    terminal_write("After IDT\n");

    __asm__ volatile (
    "xor %%rdx, %%rdx\n\t"
    "mov $10, %%rax\n\t"
    "xor %%rcx, %%rcx\n\t"
    "div %%rcx\n\t"
    :
    :
    : "rax", "rcx", "rdx"


);


    void *a = kmalloc(3);
    void *b = kmalloc(5);
    void *c = kmalloc(7);

    if (a && b && c)
    {
        terminal_write("Heap Allocation OK\n");
    }
    else
    {
        terminal_write("Heap Allocation FAILED\n");
    }

    kprintf("Hello from kprintf\n");

    kprintf("Kernel: %s\n", "ZenithOS");
    kprintf("Author: %s\n", "Divyanshu");
    kprintf("Positive = %d\n", 12345);
    kprintf("Negative = %d\n", -6789);
    kprintf("Zero = %d\n", 0);

    kprintf("Unsigned = %u\n", 123456789u);
    kprintf("Zero = %u\n", 0u);

    int value = 42;

    kprintf("Address of value : %p\n", &value);
    kprintf("Heap             : %p\n", kernel_heap);

    terminal_write("ZenithOS booted!\n");
    
    hcf();
}
