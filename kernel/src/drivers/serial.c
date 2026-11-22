#include "serial.h"
#include "../hal/io.h"
#include "../hal/pic.h"
#include "../cpu/irq.h"
#include "../cpu/interrupts.h"
#include "../fs/vfs.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"
#include "../lib/memory.h"
#include "../mm/heap.h"
#include "../stdio/printf.h"
#include "../kernel_err/errno.h"

static char rx_ring[SERIAL_RX_BUFFER_SIZE];
static volatile size_t rx_head = 0;
static volatile size_t rx_tail = 0;

static volatile uint64_t g_bytes_rx = 0;
static volatile uint64_t g_bytes_tx = 0;
static volatile uint64_t g_rx_overruns = 0;
static bool g_serial_present = false;

static void serial_irq_handler(interrupt_context_t *context)
{
    (void)context;

    // Bounded drain of up to 64 bytes per interrupt to prevent hanging in ISR
    int max_bytes = 64;
    while (max_bytes-- > 0 && (inb(COM1_PORT + 5) & 0x01))
    {
        uint8_t byte = inb(COM1_PORT + 0);
        size_t next = (rx_head + 1) % SERIAL_RX_BUFFER_SIZE;
        if (next != rx_tail)
        {
            rx_ring[rx_head] = (char)byte;
            rx_head = next;
            g_bytes_rx++;
        }
        else
        {
            g_rx_overruns++;
        }
    }
}

void serial_init(void)
{
    rx_head = 0;
    rx_tail = 0;
    g_bytes_rx = 0;
    g_bytes_tx = 0;
    g_rx_overruns = 0;

    // 1. Disable all interrupts
    outb(COM1_PORT + 1, 0x00);

    // 2. Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 3, 0x80);

    // 3. Set divisor to 1 (115200 baud)
    outb(COM1_PORT + 0, 0x01); // Divisor LSB
    outb(COM1_PORT + 1, 0x00); // Divisor MSB

    // 4. 8 bits, no parity, one stop bit (8N1)
    outb(COM1_PORT + 3, 0x03);

    // 5. Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 2, 0xC7);

    // 6. Test loopback mode to check hardware responsiveness
    outb(COM1_PORT + 4, 0x1E);
    outb(COM1_PORT + 0, 0xAE);
    uint8_t echo = inb(COM1_PORT + 0);

    if (echo != 0xAE)
    {
        // Hardware test failed or port absent
        g_serial_present = false;
        kprintf("[SERIAL] COM1 (0x%x) hardware test returned 0x%x, serial disabled\n", COM1_PORT, echo);
        return;
    }

    g_serial_present = true;

    // 7. Normal operation mode: enable IRQs, set RTS/DSR, set OUT2 to route interrupts to PIC
    outb(COM1_PORT + 4, 0x0B);

    // 8. Enable Received Data Available interrupt (bit 0)
    outb(COM1_PORT + 1, 0x01);

    // 9. Register IRQ 4 with CPU and unmask PIC line 4
    irq_register_handler(4, serial_irq_handler);
    pic_unmask_irq(4);

    kprintf("[SERIAL] COM1 initialized at 115200 8N1 (IRQ 4)\n");
}

bool serial_is_present(void)
{
    return g_serial_present;
}

void serial_putc(char c)
{
    if (!g_serial_present)
    {
        return;
    }

    // Bounded timeout checking for Transmitter Holding Register Empty (LSR bit 5)
    int timeout = 10000;
    while (timeout-- > 0 && !(inb(COM1_PORT + 5) & 0x20))
    {
        __asm__ volatile("pause");
    }

    if (timeout > 0)
    {
        outb(COM1_PORT + 0, (uint8_t)c);
        g_bytes_tx++;
    }
}

void serial_write_bytes(const char *buf, size_t len)
{
    if (buf == NULL || !g_serial_present)
    {
        return;
    }

    for (size_t i = 0; i < len; i++)
    {
        serial_putc(buf[i]);
    }
}

int serial_read_bytes(char *buf, size_t max_len)
{
    if (buf == NULL || max_len == 0)
    {
        return 0;
    }

    // If hardware has unread bytes and interrupts were delayed, drain now
    if (g_serial_present && (inb(COM1_PORT + 5) & 0x01))
    {
        int max_poll = 32;
        while (max_poll-- > 0 && (inb(COM1_PORT + 5) & 0x01))
        {
            uint8_t byte = inb(COM1_PORT + 0);
            size_t next = (rx_head + 1) % SERIAL_RX_BUFFER_SIZE;
            if (next != rx_tail)
            {
                rx_ring[rx_head] = (char)byte;
                rx_head = next;
                g_bytes_rx++;
            }
            else
            {
                g_rx_overruns++;
            }
        }
    }

    bool irq_was_enabled = interrupt_save();

    size_t count = 0;
    while (count < max_len && rx_tail != rx_head)
    {
        buf[count++] = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) % SERIAL_RX_BUFFER_SIZE;
    }

    interrupt_restore(irq_was_enabled);
    return (int)count;
}

int serial_peek(char *buf, size_t max_len)
{
    if (buf == NULL || max_len == 0)
    {
        return 0;
    }

    bool irq_was_enabled = interrupt_save();

    size_t count = 0;
    size_t curr = rx_tail;
    while (count < max_len && curr != rx_head)
    {
        buf[count++] = rx_ring[curr];
        curr = (curr + 1) % SERIAL_RX_BUFFER_SIZE;
    }

    interrupt_restore(irq_was_enabled);
    return (int)count;
}

size_t serial_available(void)
{
    if (rx_head >= rx_tail)
    {
        return rx_head - rx_tail;
    }
    return SERIAL_RX_BUFFER_SIZE - (rx_tail - rx_head);
}

void serial_get_status(serial_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    status->port = COM1_PORT;
    status->baud_rate = 115200;
    status->is_online = g_serial_present;
    status->bytes_rx = g_bytes_rx;
    status->bytes_tx = g_bytes_tx;
    status->rx_overruns = g_rx_overruns;
    status->rx_buffered = (uint32_t)serial_available();
}

/* ----------------------------------------------------
 * VFS Operations for /dev/ttyS0
 * ---------------------------------------------------- */

int serial_vfs_read(file_t *file, void *buffer, uint64_t size)
{
    (void)file;
    if (buffer == NULL)
    {
        return -KERNEL_EINVAL;
    }

    return serial_read_bytes((char *)buffer, (size_t)size);
}

int serial_vfs_write(file_t *file, const void *buffer, uint64_t size)
{
    (void)file;
    if (buffer == NULL)
    {
        return -KERNEL_EINVAL;
    }

    serial_write_bytes((const char *)buffer, (size_t)size);
    return (int)size;
}

static vnode_operations_t serial_vnode_ops = {
    .lookup = NULL,
    .create = NULL,
    .mkdir = NULL,
    .read = serial_vfs_read,
    .write = serial_vfs_write,
    .remove = NULL
};

int serial_devfs_init(void)
{
    // Create /dev directory if not present
    vfs_mkdir("/dev");

    vnode_t *dev_dir = NULL;
    int rc = vfs_lookup("/dev", &dev_dir);
    if (rc != KERNEL_SUCCESS || dev_dir == NULL)
    {
        kprintf("[SERIAL] Failed to resolve /dev node (%d)\n", rc);
        return rc;
    }

    ramfs_node_t *parent = (ramfs_node_t *)dev_dir->private_data;
    if (parent == NULL || parent->child_count >= RAMFS_MAX_CHILDREN)
    {
        return -KERNEL_ENOMEM;
    }

    // Check if ttyS0 is already registered
    vnode_t *existing = NULL;
    if (vfs_lookup("/dev/ttyS0", &existing) == KERNEL_SUCCESS)
    {
        return KERNEL_SUCCESS;
    }

    ramfs_node_t *dev_node = kmalloc(sizeof(ramfs_node_t));
    if (dev_node == NULL)
    {
        return -KERNEL_ENOMEM;
    }

    k_memset(dev_node, 0, sizeof(ramfs_node_t));
    dev_node->vnode.type = VNODE_DEVICE;
    k_strcpy(dev_node->vnode.name, "ttyS0");
    dev_node->vnode.size = 0;
    dev_node->vnode.parent = dev_dir;
    dev_node->vnode.fs = dev_dir->fs;
    dev_node->vnode.ops = &serial_vnode_ops;
    dev_node->vnode.private_data = dev_node;

    parent->children[parent->child_count++] = dev_node;
    kprintf("[SERIAL] Registered device node /dev/ttyS0\n");

    return KERNEL_SUCCESS;
}
