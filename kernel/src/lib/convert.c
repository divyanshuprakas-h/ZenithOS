#include "convert.h"

void int_to_string(int value, char *buffer)
{
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int negative = 0;

    if (value < 0)
    {
        negative = 1;
        value = -value;
    }

    int i = 0;
    while (value > 0)
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }
    
    if (negative)
    {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    int start = 0;
    int end = i - 1;

    while (start < end)
    {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        start++;
        end--;
    }
}

void uint64_to_hex(uint64_t value, char *buffer)
{
    const char digits[] = "0123456789ABCDEF";

    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int i = 0;
    while (value > 0)
    {
        buffer[i++] = digits[value & 0xF];
        value >>= 4;
    }

    buffer[i] = '\0';

    int start = 0;
    int end = i-1;

    while (start < end)
    {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        start++;
        end--;
    }
}

void uint_to_string(uint32_t value, char *buffer)
{
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int i = 0;

    while (value > 0)
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    buffer[i] = '\0';

    int start = 0;
    int end = i - 1;

    while (start < end)
    {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        start++;
        end--;
    }
}