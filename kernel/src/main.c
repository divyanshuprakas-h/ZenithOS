#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include "kernel_err/errno.h"

#include "initramfs/initramfs.h"
#include "framebuffer/framebuffer.h"
#include "terminal/terminal.h"
#include "mm/heap.h"
#include "lib/memory.h"
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

#include "vmm/vma/vma.h"

#include "scheduler/scheduler.h"
#include "tests/scheduler_test.h"
#include "tests/scheduler_mutex_test.h"
#include "tests/tests.h"

#include "process/process.h"
#include "tests/fs_test.h"
#include "process/process_exec_file.h"

#include "fs/fs_init.h"

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

__attribute__((used, section(".limine_requests")))
    static volatile struct limine_module_request module_request = 
    {
        .id = LIMINE_MODULE_REQUEST_ID,
        .revision = 0
    };

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

typedef struct user_interrupt_frame
{
    registers_t regs;

    uint64_t interrupt_number;
    uint64_t error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;

    uint64_t rsp;
    uint64_t ss;
} user_interrupt_frame_t;

static void init_process_stub(void);

static bool launch_user_process(process_t *process)
{
    if (process == NULL || !process->is_user_process)
    {
        return false;
    }

    if (process->main_thread != NULL)
    {
        return false;
    }

    task_t *task = task_create(init_process_stub);

    if (task == NULL)
    {
        return false;
    }

    task->process = process;
    process->main_thread = task;
    process->thread_list = task;

    user_interrupt_frame_t *frame =
        (user_interrupt_frame_t *)((uint8_t *)(uintptr_t)task->context.rsp -
                                    sizeof(user_interrupt_frame_t));

    k_memset(frame, 0, sizeof(*frame));

    frame->regs.rdi = (uint64_t)(uintptr_t)&task->context;
    frame->rip = process->user_rip;
    frame->rsp = process->user_rsp;
    frame->rflags = process->user_rflags;
    frame->cs = USER_CODE_SELECTOR | 3;
    frame->ss = USER_DATA_SELECTOR | 3;

    task->interrupt_rsp = frame;
    task->resume_mode = TASK_RESUME_INTERRUPT;

    process->user_started = true;

    scheduler_add_task(task);
    return true;
}

static void init_process_stub(void)
{
    for (;;)
    {
        __asm__ volatile("hlt");
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
    kprintf("[TRACE] kmain entry\n");

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

    if (module_request.response == NULL)
    {
        terminal_write("No module response!\n");
        hcf();
    }
    kprintf("Modules loaded: %llu\n", (unsigned long long)module_request.response->module_count);

    const struct limine_file *module = module_request.response->modules[0];

    kprintf("Module path : %s\n", module->path);
    kprintf("Module addr : %p\n", module->address);
    kprintf("Module size : %llu\n", (unsigned long long)module->size);

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

    if (fs_init() != KERNEL_SUCCESS)
    {
        kprintf("[FAIL] fs_init\n");
        hcf();
    }

    if (!initramfs_init(module->address, module->size))
    {
        kprintf("[FAIL] initramfs_init\n");
        hcf();
    }

    kprintf("[PASS] initramfs_init\n");

    terminal_write("Memory Initialization Complete\n");



    /* ----------------------------------------------------
     * Initialize CPU
     * ---------------------------------------------------- */


    gdt_init();


    idt_init();
    

    irq_init();

    terminal_write("CPU Initialization Complete\n");

    /* ----------------------------------------------------
     * Initialize Hardware
     * ---------------------------------------------------- */

    apic_disable();
    pic_init();
    pit_init(100);

    keyboard_init();
    mouse_init();

    terminal_write("Driver Initialization Complete\n");

    page_table_activate();

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

    kprintf("[TRACE] scheduler_init()\n");
    scheduler_init();
    kprintf("[TRACE] scheduler_init() done current_task=%p\n", scheduler_current_task());

    kprintf("[TRACE] kernel_tests() begin\n");
    kernel_tests();
    kprintf("[TRACE] kernel_tests() end\n");

    /* ----------------------------------------------------
     * Initialize GUI & Desktop Environment
     * ---------------------------------------------------- */
    gui_init();
    terminal_app_init();
    desktop_init();
    desktop_draw();

    /* ----------------------------------------------------
     * Launch /bin/init in Ring 3 User Mode
     * ---------------------------------------------------- */
    process_t *init_process = NULL;

    int rc = process_exec_file("/bin/init", &init_process);

    if (rc != KERNEL_SUCCESS)
    {
        kprintf("[FAIL] Cannot launch /bin/init (%d)\n", rc);
        hcf();
    }

    if (!launch_user_process(init_process))
    {
        kprintf("[FAIL] Cannot schedule /bin/init\n");
        hcf();
    }

    kprintf("[BOOT] init process created (pid=%u)\n", init_process->pid);

    /* ----------------------------------------------------
     * Desktop Compositor Loop (Task 0)
     * ---------------------------------------------------- */
    while (1)
    {
        if (gui_needs_redraw)
        {
            gui_needs_redraw = false;
            desktop_draw();
        }

        scheduler_yield();
    }

    hcf();
}
