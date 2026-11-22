#ifndef USER_ACCESS_H
#define USER_ACCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Validates that [ptr, ptr + size) lies strictly inside canonical user space
 * and does not overflow integer bounds.
 */
bool user_ptr_valid(const void *ptr, size_t size);

/**
 * Validates that every page in [ptr, ptr + size) is mapped, user-accessible,
 * and matches the required access type (read vs write).
 */
bool user_ptr_accessible(const void *ptr, size_t size, bool write_access);

/**
 * Copies memory from user space to kernel space after validating page mappings.
 * Returns KERNEL_SUCCESS (0) on success, or negative error code on failure.
 */
int copy_from_user(void *dst, const void *src, size_t size);

/**
 * Copies memory from kernel space to user space after validating page mappings and write permissions.
 * Returns KERNEL_SUCCESS (0) on success, or negative error code on failure.
 */
int copy_to_user(void *dst, const void *src, size_t size);

/**
 * Copies a null-terminated string from user space into a bounded kernel buffer.
 * Validates each page traversed and rejects unterminated or oversized strings.
 * Returns KERNEL_SUCCESS (0) on success, or negative error code on failure.
 */
int copy_str_from_user(char *dst, const char *src, size_t max_len);

#endif
