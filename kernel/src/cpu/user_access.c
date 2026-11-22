#include "user_access.h"
#include "../vmm/user_vm_layout.h"
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

    // Check pointer wrap-around
    if (end < start)
    {
        return false;
    }

    // Must be strictly within valid user memory window
    if (start < USER_BASE || end > USER_MAX_ADDRESS)
    {
        return false;
    }

    return true;
}

int copy_from_user(void *dst, const void *src, size_t size)
{
    if (!user_ptr_valid(src, size))
    {
        return -KERNEL_EINVAL;
    }

    k_memcpy(dst, src, size);
    return 0;
}

int copy_to_user(void *dst, const void *src, size_t size)
{
    if (!user_ptr_valid(dst, size))
    {
        return -KERNEL_EINVAL;
    }

    k_memcpy(dst, src, size);
    return 0;
}

int copy_str_from_user(char *dst, const char *src, size_t max_len)
{
    if (src == NULL || dst == NULL || max_len == 0)
    {
        return -KERNEL_EINVAL;
    }

    uintptr_t uaddr = (uintptr_t)src;
    if (uaddr < USER_BASE || uaddr >= USER_MAX_ADDRESS)
    {
        return -KERNEL_EINVAL;
    }

    size_t i = 0;
    while (i < max_len - 1)
    {
        uintptr_t curr = uaddr + i;
        if (curr >= USER_MAX_ADDRESS)
        {
            dst[i] = '\0';
            return -KERNEL_EINVAL;
        }

        char c = src[i];
        dst[i] = c;
        if (c == '\0')
        {
            return 0;
        }
        i++;
    }

    dst[max_len - 1] = '\0';
    return 0;
}
