#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../fs/vnode.h"
#include "../fs/file.h"

#define COM1_PORT 0x3F8
#define SERIAL_RX_BUFFER_SIZE 4096

typedef struct {
    uint32_t port;
    uint32_t baud_rate;
    bool is_online;
    uint64_t bytes_rx;
    uint64_t bytes_tx;
    uint64_t rx_overruns;
    uint32_t rx_buffered;
} serial_status_t;

void serial_init(void);
bool serial_is_present(void);
void serial_putc(char c);
void serial_write_bytes(const char *buf, size_t len);
int serial_read_bytes(char *buf, size_t max_len);
int serial_peek(char *buf, size_t max_len);
size_t serial_available(void);
void serial_get_status(serial_status_t *status);

// VFS device registration and file operations
int serial_vfs_read(file_t *file, void *buffer, uint64_t size);
int serial_vfs_write(file_t *file, const void *buffer, uint64_t size);
int serial_devfs_init(void);

#endif
