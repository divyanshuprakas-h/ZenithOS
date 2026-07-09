#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdint.h>

void exception_dispatch(uint64_t interrupt_number);

#endif