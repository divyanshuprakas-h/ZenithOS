#include "vma.h"

#include "../../stdio/printf.h"

#define VMA_MAX_COUNT 32

static vma_t vma_table[VMA_MAX_COUNT];
static uint32_t vma_count = 0;

void vma_init(void)
{
    vma_count = 0;

    for (uint32_t i = 0 ; i < VMA_MAX_COUNT; i++)
    {
        vma_table[i].start = 0;
        vma_table[i].end = 0;
        vma_table[i].flags = 0;
    }
}

bool vma_create(
    uint64_t start,
    uint64_t size,
    uint64_t flags
)
{

    if (vma_count >= VMA_MAX_COUNT)
    {
        return false;
    }

    uint64_t new_start = start;
    uint64_t new_end = start + size;

    for (uint32_t i = 0; i < vma_count; i++)
    {
        uint64_t existing_start = vma_table[i].start;
        uint64_t existing_end = vma_table[i].end;

        if (new_start < existing_end && new_end > existing_start)
        {
            return false;
        }
    }

    vma_table[vma_count].start = start;
    vma_table[vma_count].end = start + size;
    vma_table[vma_count].flags = flags;

    vma_count++;

    return true;
}

const vma_t *vma_find(uint64_t address)
{
    for (uint32_t i = 0; i < vma_count; i++)
    {
        if (address >= vma_table[i].start && address < vma_table[i].end)
        {
            return &vma_table[i];
        }
    }
    return NULL;
}

void vma_dump(void)
{
    kprintf("\n");
    kprintf("============== VMA TABLE ================\n");

    if (vma_count == 0)
    {
        kprintf("NO VMA's\n");
        kprintf("===============================\n");
        return;
    }

    for (uint32_t i =0 ; i < vma_count ; i++)
    {
        kprintf("VMA %u\n", i);
        kprintf("Start : %p\n", (void *)vma_table[i].start);
        kprintf("End : %p\n", (void *)vma_table[i].end);
        kprintf("Flags : ");

        if (vma_table[i].flags & VMA_READ)
            kprintf("READ");

        if (vma_table[i].flags & VMA_WRITE)
            kprintf("WRITE");

        if (vma_table[i].flags & VMA_EXECUTE)
            kprintf("EXECUTE");

        if (vma_table[i].flags & VMA_DEMAND_PAGED)
            kprintf("DEMAND");

        kprintf("\n");

    }

    kprintf("====================================\n");
}

bool vma_destroy(uint64_t start)
{
    for (uint32_t i = 0; i < vma_count; i++)
    {
        if (vma_table[i].start != start)
        {
            continue;
        }

        for (uint32_t j = i; j + 1 < vma_count; j++)
        {
            vma_table[j] = vma_table[j+1];
        }

        vma_table[vma_count - 1].start = 0;
        vma_table[vma_count - 1].end = 0;
        vma_table[vma_count - 1].flags = 0;

        vma_count--;
        return true;
    }
    return false;
}

bool vma_resize(uint64_t start, uint64_t new_size)
{
    vma_t *target = NULL;

    for (uint32_t i = 0; i < vma_count; i++)
    {
        if (vma_table[i].start == start)
        {
            target = &vma_table[i];
            break;
        }
    }

    if (target == NULL)
    {
        return false;
    }

    uint64_t new_end = start + new_size;

    for (uint32_t i = 0; i < vma_count; i++)
    {
        if (&vma_table[i] == target)
        {
            continue;
        }
        if (start < vma_table[i].end && new_end > vma_table[i].start)
        {
            return false;
        }
    }
    target->end = new_end;
    return true;
}