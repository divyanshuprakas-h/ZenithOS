#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "interrupt_context.h"

void exception_dispatch(interrupt_context_t *context);

#endif