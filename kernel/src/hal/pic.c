#include "pic.h"

#include "io.h"

static void pic_set_irq_mask(uint8_t irq_line, bool masked)
{
    uint16_t data_port;
    uint8_t irq_bit;

    if (irq_line < 8)
    {
        data_port = PIC1_DATA;
        irq_bit = irq_line;
    }
    else
    {
        data_port = PIC2_DATA;
        irq_bit = (uint8_t)(irq_line - 8);
    }

    uint8_t mask = inb(data_port);

    if (masked)
    {
        mask |= (uint8_t)(1u << irq_bit);
    }
    else
    {
        mask &= (uint8_t)~(1u << irq_bit);
    }

    outb(data_port, mask);
}

void pic_remap(uint8_t master_offset, uint8_t slave_offset)
{
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask = inb(PIC2_DATA);

    outb(PIC1_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, master_offset);
    io_wait();
    outb(PIC2_DATA, slave_offset);
    io_wait();

    outb(PIC1_DATA, 0x04u);
    io_wait();
    outb(PIC2_DATA, 0x02u);
    io_wait();

    outb(PIC1_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086);
    io_wait();

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_mask_all(void)
{
    outb(PIC1_DATA, 0xFFu);
    outb(PIC2_DATA, 0xFFu);
}

void pic_mask_irq(uint8_t irq_line)
{
    pic_set_irq_mask(irq_line, true);
}

void pic_unmask_irq(uint8_t irq_line)
{
    pic_set_irq_mask(irq_line, false);
}

void pic_send_eoi(uint8_t irq_line)
{
    if (irq_line >= 8)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_init(void)
{
    pic_remap(PIC_PRIMARY_VECTOR, PIC_SECONDARY_VECTOR);
    pic_mask_all();
}
