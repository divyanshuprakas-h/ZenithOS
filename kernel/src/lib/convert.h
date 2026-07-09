#ifndef CONVERT_H
#define CONVERT_H

#include <stdint.h>

void int_to_string(int value, char *buffer);
void uint64_to_hex(uint64_t value, char *buffer);
void uint_to_string(uint32_t value, char *buffer);

#endif