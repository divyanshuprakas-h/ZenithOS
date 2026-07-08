#include "math.h"

int abs_int(int value)
{
    return (value < 0) ? -value : value;
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int max(int a , int b)
{
    return (a > b) ? a : b;
}