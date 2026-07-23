#pragma once

#include "../interrupt_context.h"

#include <stdint.h>
#include <stdbool.h>

void page_fault_handler(interrupt_context_t *context);



