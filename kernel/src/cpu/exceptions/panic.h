#ifndef PANIC_H
#define PANIC_H

#include "../interrupt_context.h"

void kernel_panic(const char *reason, interrupt_context_t *context);

#endif