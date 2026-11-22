#ifndef SYS_FILE_H
#define SYS_FILE_H

#include <stdint.h>

int sys_open(const char *path);

int sys_close(int fd);

int sys_read(int fd, void *buffer, uint64_t size);

int sys_write(int fd, void *buffer, uint64_t size);

int sys_readdir(const char *path, void *buffer, uint64_t max_entries);

#endif