#ifndef SYSCALL_H
#define SYSCALL_H

#include "interrupt_context.h"
#include <stdint.h>

#define SYS_EXIT    0
#define SYS_OPEN    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_CLOSE   4
#define SYS_YIELD   5
#define SYS_SYSINFO 6
#define SYS_READDIR 7

typedef struct
{
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t used_pages;
    uint64_t page_size;
} sysinfo_t;

void syscall_dispatch(interrupt_context_t *context);

#endif
