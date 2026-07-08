#include "memory.h"

void *k_memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}

void *k_memset(void *s, int c, size_t n)
{
    uint8_t *ptr = (uint8_t *)s;

    for (size_t i = 0; i< n; i++)
    {
        ptr[i] = (uint8_t)c;
    }

    return s;
}

void *k_memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d < s)
    {
        for (size_t i = 0; i < n; i++)
        {
            d[i] = s[i];
        }
    }
    else if (d > s)
    {
        for (size_t i = n ; i > 0; i --)
        {
            d[i - 1] = s[i - 1];
        }
    } 

    return dest;
}

int k_memcmp(const void *s1 , const void *s2, size_t n)
{
    const uint8_t *a = (const uint8_t *)s1;
    const uint8_t *b = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++)
    {
        if (a[i] != b[i])
        {
            return a[i] - b[i];
        }
    }

    return 0;
}
