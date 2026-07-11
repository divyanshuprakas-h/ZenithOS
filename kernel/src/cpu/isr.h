#ifndef ISR_H
#define ISR_H

#include <stdint.h>

#include "interrupt_context.h"

#define ISR_VECTOR_COUNT 48

void isr_dispatch(interrupt_context_t *context);

extern void (*const isr_table[ISR_VECTOR_COUNT])(void);

#endif
