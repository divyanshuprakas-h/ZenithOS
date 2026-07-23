#pragma once

#include <stdbool.h>
#include <stdint.h>

bool vmm_handle_page_fault(
    uint64_t fault_address,
    uint64_t error_code
);

