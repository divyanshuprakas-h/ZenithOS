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

char *k_strncpy(char *dest, const char *src, size_t n)
{
    char *start = dest;
    size_t i = 0;

    for (; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }

    return start;
}

char *k_strcat(char *dest, const char *src)
{
    char *start = dest;

    while (*dest != '\0')
    {
        dest++;
    }
    while ((*dest++ = *src++) != '\0');

    return start;
}


