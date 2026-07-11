#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#include "ports.h"

#define PIT_DEFAULT_FREQUENCY_HZ 100u
#define PIT_MODE_SQUARE 0x36u

void pit_init(uint32_t frequency_hz);
uint64_t pit_ticks(void);

#endif
