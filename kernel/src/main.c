#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include "framebuffer/framebuffer.h"
#include "terminal/terminal.h"

// -------------------------------------------------------------
// Limine Base Revision
// -------------------------------------------------------------

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// -------------------------------------------------------------
// Framebuffer Request
// -------------------------------------------------------------

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// -------------------------------------------------------------
// Limine Start / End Markers
// -------------------------------------------------------------

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

// -------------------------------------------------------------
// Halt CPU
// -------------------------------------------------------------

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

// -------------------------------------------------------------
// Kernel Entry
// -------------------------------------------------------------

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

    framebuffer_clear(0x000000);

    terminal_init();

    terminal_write("====================================\n");
    terminal_write("          ZenithOS Kernel\n");
    terminal_write("====================================\n\n");

    terminal_write("Framebuffer : OK\n");
    terminal_write("Graphics    : OK\n");
    terminal_write("Font        : OK\n");
    terminal_write("Terminal    : OK\n\n");

    terminal_write("Scrolling Test Begins...\n\n");

    for (int i = 0; i < 100; i++)
    {
        terminal_write("This is a scrolling test line.\n");
    }

    hcf();
}
