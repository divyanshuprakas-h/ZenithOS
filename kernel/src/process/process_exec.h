#pragma once

#include "process.h"
#include <stddef.h>

int process_exec(process_t *process, const void *elf_image, size_t image_len);
