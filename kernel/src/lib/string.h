#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t k_strlen(const char *str);

int k_strcmp(const char *a, const char *b);

char *k_strcpy(char *dest, const char *src);

#endif