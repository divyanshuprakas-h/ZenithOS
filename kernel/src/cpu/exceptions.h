#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "interrupt_context.h"

#define EXCEPTION_DIVIDE_ERROR 0
#define EXCEPTION_INVALID_OPCODE 6
#define EXCEPTION_GENERAL_PROTECTION 13
#define EXCEPTION_PAGE_FAULT 14

void exception_dispatch(interrupt_context_t *context);

#endif