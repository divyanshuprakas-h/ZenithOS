#include "heap.h"
#include "pmm.h"

#include "../stdio/printf.h"
#include "../vmm/page_table.h"
#include "../lib/memory.h"

#include "../vmm/vma/vma.h"

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

static uint64_t heap_virtual_start;
static uint64_t heap_virtual_end;
static uint64_t heap_committed_end;

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

    return NULL;
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

static void coalesce_free_blocks(void)
{
    heap_block_t *current = heap_head;

    while (current != NULL)
    {
        while (current->free && current->next != NULL && current->next->free)
        {
            merge_with_next(current);
        }

        current = current->next;
    }
}

#define HEAP_ALIGNMENT 16

static size_t align_size(size_t size)
{
    return(size + (HEAP_ALIGNMENT - 1)) & 
            ~(HEAP_ALIGNMENT - 1); 
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

static const vma_t *heap_vma = NULL;

static void *heap_commit_page(uint64_t virtual_address)
{
    void *physical = pmm_alloc_page();
    if (physical == NULL)
        return NULL;

    if (!page_table_map_page(
        virtual_address,
        (uint64_t)(uintptr_t)physical,
        true
    ))
    {
        return NULL;
    }
    heap_committed_end += PAGE_SIZE;
    
    return (void *)virtual_address;
}

static heap_block_t *heap_create_block(
    uint64_t virtual_address,
    size_t size
)
{
    heap_block_t *block = (heap_block_t *)virtual_address;

    block->size = size - sizeof(heap_block_t);
    block->free = true;
    block->next = NULL;
    block->prev = NULL;

    return block;
}

static inline uint64_t heap_committed_size(void)
{
    return heap_committed_end - heap_virtual_start;
}

static inline uint64_t heap_next_commit_address(void)
{
    return heap_committed_end;
}

static bool heap_grow(size_t bytes)
{
    while (heap_committed_size() < bytes)
    {
        if (!heap_expand())
        {
            return false;
        }
    }
    return true;
}

bool heap_expand(void)
{
    if (heap_vma == NULL)
    {
        kprintf("Heap VMA missing!\n");
        return false;
    }

    uint64_t new_size = heap_committed_size() + HEAP_GROW_SIZE;
    uint64_t commit_address = heap_next_commit_address();

    kprintf("========== Heap Expand ==========\n");
    kprintf("Current Heap Size : %u bytes\n", (unsigned)heap_committed_size());
    kprintf("Requested Size    : %u bytes\n", (unsigned)new_size);

    void *virtual_page = heap_commit_page(commit_address);

    if (virtual_page == NULL)
    {
        return false;
    }

    heap_block_t *last = find_last_block();
    heap_block_t *new_block = heap_create_block(
        (uint64_t)virtual_page,
        PAGE_SIZE
    );

    last->next = new_block;
    new_block->prev = last;

    if (last->free)
    {
        merge_with_next(last);
    }

    kprintf("Heap Expanded Successfully!\n");
    kprintf("Heap Expand: %p\n", virtual_page);

    heap_size = heap_committed_end - heap_virtual_start;
    heap_virtual_end = heap_committed_end;

    if (!vma_resize(heap_vma->start, heap_size))
    {
        kprintf("Heap VMA resize failed!\n");
    }

    kprintf("Committed End : %p\n", (void *)heap_committed_end);
    kprintf("Heap Size : %u\n", (unsigned)heap_size);

    return true;
}

void heap_init(void)
{
    void *heap_physical = pmm_alloc_page();

    if (heap_physical == NULL)
    {
        kprintf("Heap: Failed to allocate first page\n");
        return;
    }

    if (!page_table_map_page(
        HEAP_START_ADDRESS,
        (uint64_t)(uintptr_t)heap_physical,
        true
    ))
    {
        kprintf("HEAP FAILED to map first HEAP PAGE\n");
        return;
    }

    heap_virtual_start = HEAP_START_ADDRESS;

    heap_virtual_end = HEAP_START_ADDRESS + HEAP_INITIAL_SIZE;

    heap_committed_end = HEAP_START_ADDRESS + HEAP_INITIAL_SIZE;

    heap_start = (void *)HEAP_START_ADDRESS;

    heap_vma = vma_find(HEAP_START_ADDRESS);
    if (heap_vma == NULL)
    {
        kprintf("Heap VMA not found!\n");
        return;
    }

    heap_size = HEAP_INITIAL_SIZE;

    heap_head = (heap_block_t *)heap_start;

    heap_head->size = heap_size - sizeof(heap_block_t);

    heap_head->free = true;

    heap_head->next = NULL;

    heap_head->prev = NULL;

    kprintf("================================\n");

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

    while (true)
    {
        coalesce_free_blocks();

        heap_block_t *block = find_free_block(size);
        if (block != NULL)
        {
            split_block(block, size);
            block->free = false;

            return (void *)(block + 1);
        }

        if (!heap_grow(heap_committed_size() + HEAP_GROW_SIZE))
        {
            return NULL;
        }
    }

}

void *kcalloc(size_t count, size_t size)
{
    if (count == 0 || size == 0)
    {
        return NULL;
    }

    if (count > SIZE_MAX / size)
    {
        return NULL;
    }

    size_t total = count * size;

    void *ptr = kmalloc(total);

    if (ptr == NULL)
    {
        return NULL;
    }

    k_memset(ptr, 0, total);
    return ptr;
}

void *krealloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return kmalloc(size);
    }

    if (size == 0)
    {
        kfree(ptr);
        return NULL;
    }

    size = align_size(size);
    heap_block_t *block = ((heap_block_t *)ptr) - 1;

    if (block->size >= size)
    {
        return ptr;
    }

    void *new_ptr = kmalloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }

    k_memcpy(new_ptr, ptr, block->size);
    kfree(ptr);

    return new_ptr;
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
