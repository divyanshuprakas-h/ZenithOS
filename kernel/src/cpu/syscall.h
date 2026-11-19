#ifndef SYSCALL_H
#define SYSCALL_H

#include "interrupt_context.h"
#include <stdint.h>

#define SYS_EXIT   0
#define SYS_OPEN   1
#define SYS_READ   2
#define SYS_WRITE  3
#define SYS_CLOSE  4
#define SYS_YIELD  5

void syscall_dispatch(interrupt_context_t *context);

#endif
