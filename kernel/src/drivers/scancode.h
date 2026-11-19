#ifndef ZENITH_SCANCODE_H
#define ZENITH_SCANCODE_H

#include <stdint.h>
#include <stdbool.h>

char scancode_to_ascii(uint8_t scancode, bool shift);

#endif
