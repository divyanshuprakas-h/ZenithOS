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
#include "cpu/exceptions_test.h"

#include "gui/gui.h"
#include "apps/terminal_app.h"
#include "gui/desktop.h"
#include "gui/gui_state.h"

#include "tests/tests.h"
#include "vmm/vma/vma.h"

#include "tests/scheduler_test.h"
#include "scheduler/task.h"
#include "scheduler/context.h"
#include "scheduler/scheduler.h"

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

static task_t *taskA = NULL;

// Kernel Entry

// static uint8_t kernel_heap[1024 * 1024];

void kmain(void)
{
    __asm__ volatile("cli");

    /* ----------------------------------------------------
     * Verify Limine Boot Requests
     * ---------------------------------------------------- */

    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision))

        hcf();


    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1)

        hcf();


    if (memmap_request.response == NULL)

        hcf();


    if (executable_address_request.response == NULL)

        hcf();
        

    if (hhdm_request.response == NULL)

        hcf();

    /* ----------------------------------------------------
     * Initialize Framebuffer
     * ---------------------------------------------------- */

    struct limine_framebuffer *framebuffer =
        framebuffer_request.response->framebuffers[0];

    framebuffer_init(
        framebuffer->address,
        framebuffer->width,
        framebuffer->height,
        framebuffer->pitch);

    framebuffer_clear(0x000000);

    terminal_init();

    terminal_write("========================================\n");
    terminal_write("              ZenithOS\n");
    terminal_write("========================================\n\n");

    /* ----------------------------------------------------
     * Initialize Memory Subsystem
     * ---------------------------------------------------- */

    memory_map_init(memmap_request.response);

    memory_map_dump();

    pmm_init(
        memmap_request.response,
        executable_address_request.response);

    page_table_set_hhdm_offset(
        hhdm_request.response->offset);

    paging_init();

    vma_init();

    vma_create(
        HEAP_START_ADDRESS,
        HEAP_INITIAL_SIZE,
        VMA_READ |
        VMA_WRITE |
        VMA_DEMAND_PAGED
    );

    heap_init();

    // for (int i = 0; i < 300; i++)
    // {
    //     kmalloc(32);
    // }
    
    bool overlap = vma_create(
        HEAP_START_ADDRESS + 0x800,
        4096,
        VMA_READ |
        VMA_WRITE |
        VMA_DEMAND_PAGED
    );
    kprintf("Overlap Test : %s\n", overlap ? "FAILED" : "PASSED");

    // bool destroyed = vma_destroy(0xFFFF900000000000ULL);
    // kprintf("Destroy Test : %s\n", destroyed ? "PASSED" : "FAILED");

    vma_dump();

    bool resized = vma_resize(
        0xFFFF900000000000ULL,
        32ULL * 1024 * 1024
    );
    kprintf("Resize Test : %s\n", resized ? "PASSED" : "FAILED");

    vma_dump();

    terminal_write("Memory Initialization Complete\n");



    /* ----------------------------------------------------
     * Initialize CPU
     * ---------------------------------------------------- */


    // gdt_init();


    // idt_init();
    

    // irq_init();

    // terminal_write("CPU Initialization Complete\n");

    /* ----------------------------------------------------
     * Initialize Hardware
     * ---------------------------------------------------- */

    // apic_disable();
    // pic_init();

    // keyboard_init();
    // mouse_init();

    // terminal_write("Driver Initialization Complete\n");

    /* ----------------------------------------------------
     * Run Kernel Tests
     * ---------------------------------------------------- */

    // page_table_activate();

    // kernel_tests();

    /* ----------------------------------------------------
     * Enable Interrupts
     * ---------------------------------------------------- */

    __asm__ volatile("sti");

    terminal_write("\nInterrupts Enabled\n");

    // volatile uint64_t *ptr = (uint64_t *)0xFFFF900000100000ULL;
    // *ptr = 0x12345678;

    terminal_write("ZenithOS Ready.\n");

    /* ----------------------------------------------------
     * Scheduler Test
     * ---------------------------------------------------- */

    taskA = task_create(task_a);
    task_t *b = task_create(task_b);

    scheduler_add_task(taskA);
    scheduler_add_task(b);

    scheduler_yield();

    /* ----------------------------------------------------
     * GUI (Enable Later)
     * ---------------------------------------------------- */

    /*
    gui_init();

    terminal_app_init();

    desktop_init();

    desktop_draw();

    while (1)
    {
        if (gui_needs_redraw)
        {
            desktop_draw();
            gui_needs_redraw = false;
        }

        __asm__ volatile("hlt");
    }
    */

    hcf();
}

