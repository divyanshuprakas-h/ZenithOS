#include "heap.h"
#include "pmm.h"

#include "../stdio/printf.h"
#include "../vmm/page_table.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct heap_block
{
    size_t size;
    bool free;
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

typedef struct 
{
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t allocations;
    size_t expansions;
} heap_stats_t;

static heap_block_t *heap_head = NULL;
static void *heap_start = NULL;
static size_t heap_size = 0;

static heap_block_t *find_free_block(size_t size)
{
    heap_block_t *current = heap_head;

    while (current != NULL)
    {
        if (current->free && current->size >= size)
        {
            return current;
        }
        current = current->next;
    }
}

static void split_block(heap_block_t *block, size_t size)
{
    if (block->size <= size + sizeof(heap_block_t))
    {
        return;
    }

    heap_block_t *new_block = (heap_block_t *)((uint8_t *)(block + 1) + size);

    new_block->size = block->size - size - sizeof(heap_block_t);

    new_block->free = true;
    new_block->next = block->next;
    new_block->prev = block;

    if (block->next)
    {
        block->next->prev = new_block;
    }

    block->next = new_block;

    block->size = size;
}

static void merge_with_next(heap_block_t *block)
{
    if (block == NULL)
    {
        return;
    }

    heap_block_t *next = block->next;

    if (next == NULL)
    {
        return;
    }

    if (!next->free)
    {
        return;
    }

    block->size += sizeof(heap_block_t) + next->size;

    block->next = next->next;

    if (next->next != NULL)
    {
        next->next->prev = block;
    }
}

static void merge_with_previous(heap_block_t *block)
{
    if (block == NULL)
    {
        return;
    }

    heap_block_t *prev = block->prev;

    if (prev == NULL)
    {
        return;
    }

    if (!prev->free)
    {
        return;
    }

    prev->size += sizeof(heap_block_t) + block->size;

    prev->next = block->next;

    if (block->next != NULL)
    {
        block->next->prev = prev;
    }

}

#define HEAP_ALIGNMENT 16

static size_t align_size(size_t size)
{
    return(size + (HEAP_ALIGNMENT - 1) & ~(HEAP_ALIGNMENT - 1));
}

static heap_block_t *find_last_block(void)
{
    heap_block_t *current = heap_head;

    while (current->next != NULL)
    {
        current = current->next;
    }

    return current;
}

bool heap_expand(void)
{
    void *physical_page = pmm_alloc_page();

    if (physical_page == NULL)
    {
        return false;
    }

    void *virtual_page = page_table_physical_to_virtual((uint64_t)(uintptr_t)physical_page);

    heap_block_t *new_block = (heap_block_t *)virtual_page;

    new_block->size = 4096 - sizeof(heap_block_t);
    new_block->free = true;
    new_block->next = NULL;
    new_block->prev = NULL;

    heap_block_t *last = find_last_block();

    last->next = new_block;
    new_block->prev = last;

    if (last->free)
    {
        merge_with_next(last);
    }

    kprintf("Heap Expanded Successfully!\n");
    kprintf("Heap Expand: %p\n", virtual_page);

    return true;
}


#define HEAP_INITIAL_SIZE 4096

void heap_init(void)
{
    void *heap_physical = pmm_alloc_page();

    if (heap_physical == NULL)
    {
        kprintf("Heap: Failed to allocate first page\n");
        return;
    }

    heap_start = page_table_physical_to_virtual((uint64_t)(uintptr_t)heap_physical);

    heap_size = HEAP_INITIAL_SIZE;

    heap_head = (heap_block_t *)heap_start;

    heap_head->size = heap_size - sizeof(heap_block_t);

    heap_head->free = true;

    heap_head->next = NULL;

    heap_head->prev = NULL;

    kprintf("Heap Initialized\n");
    kprintf("Heap Start    :%p\n", heap_start);
    kprintf("Heap Size     :%u bytes\n", (unsigned)heap_size);
    kprintf("First Free    :%u bytes\n", (unsigned)heap_head->size);

}

void *kmalloc(size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    size = align_size(size);

    heap_block_t *block = find_free_block(size);

    while ((block = find_free_block(size)) == NULL)
    {
        if (!heap_expand())
        {
            return NULL;
        }
    }

    split_block(block, size);

    block->free = false;

    return (void *)(block + 1);

}

void *kcalloc(size_t count, size_t size)
{
    (void)count;
    (void)size;
    return NULL;
}

void *krealloc(void *ptr, size_t size)
{
    (void)ptr;
    (void)size;
    return NULL;
}

void kfree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    heap_block_t *block = ((heap_block_t *)ptr) - 1;

    block->free = true;

    merge_with_next(block);
    merge_with_previous(block);

}
