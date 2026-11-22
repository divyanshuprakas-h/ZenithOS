#ifndef USER_ACCESS_H
#define USER_ACCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Validates that [ptr, ptr + size) lies strictly inside canonical user space.
 */
bool user_ptr_valid(const void *ptr, size_t size);

/**
 * Copies memory from user space to kernel space after bounds validation.
 * Returns 0 on success, negative error code on failure.
 */
int copy_from_user(void *dst, const void *src, size_t size);

/**
 * Copies memory from kernel space to user space after bounds validation.
 * Returns 0 on success, negative error code on failure.
 */
int copy_to_user(void *dst, const void *src, size_t size);

/**
 * Copies a null-terminated string from user space into a bounded kernel buffer.
 * Returns 0 on success, negative error code on failure.
 */
int copy_str_from_user(char *dst, const char *src, size_t max_len);

#endif
