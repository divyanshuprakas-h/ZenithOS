#include "string.h"

size_t k_strlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0')
    {
        len++;
    }

    return len;
}

int k_strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

char *k_strcpy(char *dest , const char *src)
{
    char *start = dest;

    while ((*dest++ = *src++) != '\0');

    return start;
}