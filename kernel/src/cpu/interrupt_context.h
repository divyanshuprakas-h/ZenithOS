#ifndef INTERRUPT_CONTEXT_H
#define INTERRUPT_CONTEXT_H

#include <stdint.h>

#include "registers.h"
#include "interrupt_frame.h"

typedef struct interrupt_context
{
    registers_t regs;
    uint64_t interrupt_number;
    uint64_t error_code;
    interrupt_frame_t frame;
} interrupt_context_t;

#endif