#include "mouse.h"

#include "../cpu/irq.h"
#include "../hal/io.h"
#include "../hal/ports.h"
#include "../hal/pic.h"
#include "../framebuffer/framebuffer.h"
#include "../gui/gui_state.h"
#include "../gui/cursor.h"
#include "../gui/window_manager.h"

MouseState mouse_state = 
{
    .x = 0,
    .y = 0,

    .left = false,
    .right = false,
    .middle =false
};

static uint8_t mouse_packet[3];
static uint8_t packet_index = 0;

static void mouse_irq_handler(interrupt_context_t *context)
{
    (void)context;
    mouse_handler();
}

static void mouse_wait_write(void)
{
    while (inb(KEYBOARD_STATUS) & 0x02)
    {

    }
    
}

static void mouse_wait_read(void)
{
    while (!(inb(KEYBOARD_STATUS) & 0x01))
    {

    }
}

static void mouse_write(uint8_t data)
{
    mouse_wait_write();
    outb(KEYBOARD_COMMAND, 0xD4);
    mouse_wait_write();
    outb(KEYBOARD_DATA, data);
}

static uint8_t mouse_read(void)
{
    mouse_wait_read();
    return inb(KEYBOARD_DATA);
}

void mouse_init(void)
{
    irq_register_handler(IRQ_MOUSE_LINE, mouse_irq_handler);

    pic_unmask_irq(IRQ_MOUSE_LINE);
    pic_unmask_irq(2);

    mouse_wait_write();
    outb(KEYBOARD_COMMAND, 0xA8);

    mouse_wait_write();
    outb(KEYBOARD_COMMAND, 0x20);

    uint8_t status = mouse_read();

    status |= 0x02;

    mouse_wait_write();
    outb(KEYBOARD_COMMAND, 0x60);

    mouse_wait_write();
    outb(KEYBOARD_DATA, status);

    mouse_write(0xF6);

    if (mouse_read() != 0xFA)
    {
        return;
    }

    mouse_write(0xF4);

    if (mouse_read() != 0xFA)
    {
        return;
    }
}

void mouse_handler(void)
{
    uint8_t status = inb(KEYBOARD_STATUS);

    if ((status & 0x21) != 0x21)
        return;

    mouse_packet[packet_index++] = inb(KEYBOARD_DATA);

    if (packet_index < 3)
        return;

    packet_index = 0;

    bool prev_left = mouse_state.left;
    bool new_left = (mouse_packet[0] & 0x01) != 0;

    mouse_state.left = new_left;
    mouse_state.right = (mouse_packet[0] & 0x02) != 0;
    mouse_state.middle = (mouse_packet[0] & 0x04) != 0;

    if (!(mouse_packet[0] & 0x08))
    {
        return;
    }

    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];

    mouse_state.x += dx;
    mouse_state.y -= dy;

    if (mouse_state.x < 0)
        mouse_state.x = 0;

    if (mouse_state.y < 0)
        mouse_state.y = 0;

    if (mouse_state.x >= (int)framebuffer_width())
        mouse_state.x = (int)framebuffer_width() - 1;

    if (mouse_state.y >= (int)framebuffer_height())
        mouse_state.y = (int)framebuffer_height() - 1;

    if (new_left && !prev_left)
    {
        window_manager_mouse_down(mouse_state.x, mouse_state.y);
    }
    else if (!new_left && prev_left)
    {
        window_manager_mouse_up();
    }

    if (dx != 0 || dy != 0)
    {
        window_manager_mouse_move(mouse_state.x, mouse_state.y);
    }

    if (!gui_needs_redraw)
    {
        cursor_move(mouse_state.x, mouse_state.y);
    }
}
