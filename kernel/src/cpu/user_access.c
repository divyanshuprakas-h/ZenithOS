#include "user_access.h"
#include "../vmm/user_vm_layout.h"
#include "../vmm/page_table.h"
#include "../kernel_err/errno.h"
#include "../lib/memory.h"
#include "../lib/string.h"

bool user_ptr_valid(const void *ptr, size_t size)
{
    if (ptr == NULL && size > 0)
    {
        return false;
    }

    if (size == 0)
    {
        return true;
    }

    uintptr_t start = (uintptr_t)ptr;
    uintptr_t end = start + size;

    // Check pointer wrap-around / arithmetic overflow
    if (end < start)
    {
        return false;
    }

    // Must be strictly within valid canonical user space
    if (start < USER_BASE || end > USER_MAX_ADDRESS)
    {
        return false;
    }

    return true;
}

bool user_ptr_accessible(const void *ptr, size_t size, bool write_access)
{
    if (size == 0)
    {
        return true;
    }

    if (!user_ptr_valid(ptr, size))
    {
        return false;
    }

    uintptr_t start = (uintptr_t)ptr;
    uintptr_t end = start + size;

    uint64_t cur_page = start & ~(PAGE_SIZE - 1);
    uint64_t last_page = (end - 1) & ~(PAGE_SIZE - 1);

    page_table_t *pml4 = page_table_get_pml4();
    if (pml4 == NULL)
    {
        return false;
    }

    while (cur_page <= last_page)
    {
        if (!page_table_check_user_access(pml4, cur_page, write_access))
        {
            return false;
        }

        // Advance to next page, checking for overflow
        if (cur_page > UINT64_MAX - PAGE_SIZE)
        {
            break;
        }
        cur_page += PAGE_SIZE;
    }

    return true;
}

int copy_from_user(void *dst, const void *src, size_t size)
{
    if (size == 0)
    {
        return KERNEL_SUCCESS;
    }

    if (dst == NULL || src == NULL)
    {
        return KERNEL_EINVAL;
    }

    if (!user_ptr_accessible(src, size, false))
    {
        return KERNEL_EINVAL;
    }

    k_memcpy(dst, src, size);
    return KERNEL_SUCCESS;
}

int copy_to_user(void *dst, const void *src, size_t size)
{
    if (size == 0)
    {
        return KERNEL_SUCCESS;
    }

    if (dst == NULL || src == NULL)
    {
        return KERNEL_EINVAL;
    }

    if (!user_ptr_accessible(dst, size, true))
    {
        return KERNEL_EINVAL;
    }

    k_memcpy(dst, src, size);
    return KERNEL_SUCCESS;
}

int copy_str_from_user(char *dst, const char *src, size_t max_len)
{
    if (src == NULL || dst == NULL || max_len == 0)
    {
        return KERNEL_EINVAL;
    }

    uintptr_t uaddr = (uintptr_t)src;
    if (uaddr < USER_BASE || uaddr >= USER_MAX_ADDRESS)
    {
        dst[0] = '\0';
        return KERNEL_EINVAL;
    }

    page_table_t *pml4 = page_table_get_pml4();
    if (pml4 == NULL)
    {
        dst[0] = '\0';
        return KERNEL_EINVAL;
    }

    uint64_t current_page = (uint64_t)-1;

    size_t i = 0;
    while (i < max_len - 1)
    {
        uintptr_t curr = uaddr + i;
        if (curr >= USER_MAX_ADDRESS)
        {
            dst[0] = '\0';
            return KERNEL_EINVAL;
        }

        uint64_t page = curr & ~(PAGE_SIZE - 1);
        if (page != current_page)
        {
            if (!page_table_check_user_access(pml4, page, false))
            {
                dst[0] = '\0';
                return KERNEL_EINVAL;
            }
            current_page = page;
        }

        char c = src[i];
        dst[i] = c;
        if (c == '\0')
        {
            return KERNEL_SUCCESS;
        }
        i++;
    }

    // If max_len was reached without a null terminator, reject as unterminated
    dst[0] = '\0';
    return KERNEL_EINVAL;
}
