#pragma once

#include <stdint.h>

#define VFS_MAX_COMPONENT_LEN   64

int path_next_component(const char **path, char *component);