#include "process_loader.h"

#include "../mm/pmm.h"
#include "../vmm/page_table.h"
#include "../kernel_err/errno.h"
#include "../stdio/printf.h"
#include "../lib/memory.h"

int process_map_segment(
    process_t *process,
    const elf64_program_header_t *ph,
    const void *elf_base
)
{
    if (process == NULL)
        return KERNEL_EINVAL;

    if (ph == NULL)
        return KERNEL_EINVAL;

    if (elf_base == NULL)
        return KERNEL_EINVAL;

    uint64_t page_count = (ph->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

    kprintf("[ELF] Segment requires %llu page(s)\n", (unsigned long long)page_count);

    for (uint64_t i = 0; i < page_count ; i++)
    {
        void *page = pmm_alloc_page();

        if (page == NULL)
            return KERNEL_ENOMEM;

        uint64_t va = ph->p_vaddr + (i * PAGE_SIZE);
        uint64_t page_offset = i * PAGE_SIZE;

        if (!page_table_map_user(
            process->page_table,
            va,
            (uint64_t)(uintptr_t)page,
            (ph->p_flags & PF_W) != 0
        ))
        {
            pmm_free_page(page);
            return KERNEL_ENOMEM;
        }

        void *page_va = page_table_physical_to_virtual((uint64_t)(uintptr_t)page);

        uint64_t bytes_to_copy = 0;
        if (page_offset < ph->p_filesz)
        {
            bytes_to_copy = ph->p_filesz - page_offset;

            if (bytes_to_copy > PAGE_SIZE)
                bytes_to_copy = PAGE_SIZE;
        }

        const uint8_t *src = (const uint8_t *)elf_base + ph->p_offset + page_offset;

        if (bytes_to_copy > 0)
        {
            k_memcpy(page_va, src, bytes_to_copy);
        }

        uint64_t bytes_in_memory = 0;

        if (page_offset < ph->p_memsz)
        {
            bytes_in_memory = ph->p_memsz - page_offset;

            if (bytes_in_memory > PAGE_SIZE)
                bytes_in_memory = PAGE_SIZE;
        }

        if (bytes_in_memory > bytes_to_copy)
        {
            k_memset(
                (uint8_t *)page_va + bytes_to_copy,
                0,
                bytes_in_memory - bytes_to_copy
            );
        }

        kprintf("[ELF] page %llu VA=0x%llx PA=%p copied=%llu zero=%llu\n", (unsigned long long)i, (unsigned long long)va, 
                page, (unsigned long long)bytes_to_copy, (unsigned long long)(bytes_in_memory - bytes_to_copy));

    }

    return KERNEL_SUCCESS;

}