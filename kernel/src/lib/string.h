#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t k_strlen(const char *str);

int k_strcmp(const char *a, const char *b);

char *k_strcpy(char *dest, const char *src);

char *k_strncpy(char *dest, const char *src, size_t n);

char *k_strcat(char *dest, const char *src);

#endif
