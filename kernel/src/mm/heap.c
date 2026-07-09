#include "heap.h"

static uint8_t *heap_start;
static uint8_t *heap_current;
static uint8_t *heap_end;

void heap_init(void *start, size_t size)
{
    heap_start = (uint8_t *)start;
    heap_current = (uint8_t *)start;
    heap_end = heap_start + size;
}

static size_t align_up(size_t value, size_t allignment)
{
    return (value + allignment - 1) & ~(allignment - 1);
}

void *kmalloc(size_t size)
{
    if (heap_current + size > heap_end)
    {
        return NULL;
    }

    void *ptr = heap_current;
    size = align_up(size, 8);
    heap_current += size;

    return ptr;

}