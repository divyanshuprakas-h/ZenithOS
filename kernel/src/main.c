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
#include "cpu/irq.h"
#include "hal/apic.h"
#include "hal/pic.h"
#include "hal/pit.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "mm/memory_map.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "vmm/page_table.h"
#include "vmm/paging.h"

#include "gui/gui.h"
#include "apps/terminal_app.h"
#include "gui/desktop.h"
#include "gui/gui_state.h"

// Limine Base Revision

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// Framebuffer Request

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

//Memory map Request
 __attribute__((used, section(".limine_requests")))
    static volatile struct limine_memmap_request memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST_ID,
        .revision = 0
    };

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request executable_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
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

static void pmm_run_tests(void)
{
    terminal_write("\n===== PMM Allocation Test =====\n");

    void *page1 = pmm_alloc_page();
    void *page2 = pmm_alloc_page();
    void *page3 = pmm_alloc_page();

    kprintf("Page1 : %p\n", page1);
    kprintf("Page2 : %p\n", page2);
    kprintf("Page3 : %p\n", page3);

    pmm_free_page(page2);

    terminal_write("Freed Page2\n");

    void *page4 = pmm_alloc_page();

    kprintf("Page4 : %p\n", page4);
    kprintf("Free Pages : %u\n", (unsigned)pmm_get_free_pages());
    kprintf("Used Pages : %u\n", (unsigned)pmm_get_used_pages());

    kprintf("\n");
    kprintf("===== Lock Test =====\n");

    kprintf("Free Pages : %u\n", (unsigned)pmm_get_free_pages());

    pmm_lock_page((void *)0x1000);
    kprintf("After Lock : %u\n", (unsigned)pmm_get_free_pages());

    pmm_unlock_page((void *)0x1000);
    kprintf("After Unlock : %u\n", (unsigned)pmm_get_free_pages());
}

// Kernel Entry

// static uint8_t kernel_heap[1024 * 1024];

void kmain(void)
{
    __asm__ volatile("cli");

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

    if (memmap_request.response == NULL)
    {
        hcf();
    }

    if (executable_address_request.response == NULL)
    {
        hcf();
    }

    if (hhdm_request.response == NULL)
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

    framebuffer_clear(0x000000);

    terminal_init();

    // terminal_write("====================================\n");
    // terminal_write("          ZenithOS Kernel\n");
    // terminal_write("====================================\n\n");

    memory_map_init(memmap_request.response);

    memory_map_dump();

    pmm_init(
        memmap_request.response,
        executable_address_request.response
    );

    page_table_set_hhdm_offset(hhdm_request.response->offset);

    heap_init();
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    void *c = kmalloc(256);

    kprintf("Allocated 1 = %p\n", a);
    kprintf("Allocated 2 = %p\n", b);
    kprintf("Allocated 3= %p\n", c);

    kfree(a);
    terminal_write("Freed A\n");

    kfree(b);
    terminal_write("Freed B\n");

    kfree(c);
    terminal_write("Freed C\n");

    void *d = kmalloc(600);
    kprintf("D = %p\n", d);

    terminal_write("Heap OK\n");


    uint32_t *arr = (uint32_t *)kcalloc(16, sizeof(uint32_t));
    for (int i =0 ; i < 16; i++)
    {
        kprintf("%u ", arr[i]);
    }
    kprintf("\n");

    arr[5] = 1234;
    for (int i = 0; i < 16; i++)
    {
        kprintf("%u ", arr[i]);
    }
    kprintf("\n");

    char *str = kmalloc(16);
    str[0] = 'H';
    str[1] = 'i';
    str[2] = '\0';

    kprintf("%s\n",str);
    str = krealloc(str, 64);
    kprintf("%s\n", str);

    // void *ptrs[500];

    // for (int i = 0; i < 500; i++)
    // {
    //     ptrs[i] = kmalloc(64);
    // }

    // for (int i = 0; i < 500; i += 2)
    // {
    //     kfree(ptrs[i]);
    // }

    // for (int i = 0; i < 250; i++)
    // {
    //     ptrs[i] = kmalloc(64);

    //     if (ptrs[i] == NULL)
    //     {
    //         kprintf("Reallocation failed!\n");
    //         break;
    //     }
    // }

    // kprintf("Heap stress test passed!\n");

    // paging_init();

    // kprintf("Kernel Physical : %p\n",
    // (void *)executable_address_request.response->physical_base);

    // kprintf("Kernel Virtual  : %p\n",
    // (void *)executable_address_request.response->virtual_base);

    // pmm_run_tests();

    // terminal_write("Framebuffer : OK\n");
    // terminal_write("Graphics    : OK\n");
    // terminal_write("Font        : OK\n");
    // terminal_write("Terminal    : OK\n\n");

    // terminal_write("Before GDT\n");

    gdt_init();
    terminal_write("GDT OK\n");

    // terminal_write("After GDT\n");

    idt_init();
    terminal_write("IDT OK\n");

    // terminal_write("After IDT\n");

    irq_init();
    terminal_write("IRQ OK\n");

    apic_disable();
    terminal_write("APIC OK\n");

    pic_init();
    terminal_write("PIC OK\n");

    // pit_init(PIT_DEFAULT_FREQUENCY_HZ);
    keyboard_init();
    terminal_write("Keyboard OK\n");

    mouse_init();
    terminal_write("Mouse OK\n");

    // terminal_write("After HAL\n");


    // void *a = kmalloc(3);
    // void *b = kmalloc(5);
    // void *c = kmalloc(7);

    // if (a && b && c)
    // {
    //     terminal_write("Heap Allocation OK\n");
    // }
    // else
    // {
    //     terminal_write("Heap Allocation FAILED\n");
    // }

    // kprintf("Hello from kprintf\n");

    // kprintf("Kernel: %s\n", "ZenithOS");
    // kprintf("Author: %s\n", "Divyanshu");
    // kprintf("Positive = %d\n", 12345);
    // kprintf("Negative = %d\n", -6789);
    // kprintf("Zero = %d\n", 0);

    // kprintf("Unsigned = %u\n", 123456789u);
    // kprintf("Zero = %u\n", 0u);

    // int value = 42;

    // kprintf("Address of value : %p\n", &value);
    // kprintf("Heap             : %p\n", kernel_heap);

    // terminal_write("ZenithOS booted!\n");
    // terminal_write("Interrupts armed\n");

    __asm__ volatile("sti");
    terminal_write("STI OK\n");

   
    
    // gui_init();
    // terminal_write("GUI OK\n");

    // terminal_app_init();
    // terminal_write("Terminal App OK\n");

    // desktop_init();
    // terminal_write("Desktop Init OK\n");

    // desktop_draw();
    // terminal_write("Desktop Draw OK\n");

    // while (1)
    // {
    //     if (gui_needs_redraw)
    //     {
    //         desktop_draw();
    //         gui_needs_redraw = false;
    //     }

    //     __asm__ volatile("hlt");
    // }

    // kprintf("Width  : %u\n", (unsigned)framebuffer_width());
    // kprintf("Height : %u\n", (unsigned)framebuffer_height());
    // kprintf("Pitch  : %u\n", (unsigned)framebuffer_pitch());

    hcf();
}
