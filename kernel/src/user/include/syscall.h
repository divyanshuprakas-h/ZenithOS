#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SYS_EXIT    0
#define SYS_OPEN    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_CLOSE   4
#define SYS_YIELD   5
#define SYS_SYSINFO 6
#define SYS_READDIR 7
#define SYS_CREATE  8
#define SYS_MKDIR   9
#define SYS_UNLINK  10

typedef struct
{
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t used_pages;
    uint64_t page_size;
} sysinfo_t;

typedef struct
{
    char name[64];
    uint32_t type; // 0 = file, 1 = directory
    uint64_t size;
} dirent_t;

long syscall0(long number);
long syscall1(long number, long arg1);
long syscall2(long number, long arg1, long arg2);
long syscall3(long number, long arg1, long arg2, long arg3);

int open(const char *path);
int read(int fd, void *buf, uint64_t count);
int write(int fd, const void *buf, uint64_t count);
int close(int fd);
void exit(int status);
void yield(void);
int sysinfo(sysinfo_t *info);
int readdir(const char *path, dirent_t *entries, uint64_t max_entries);
int create(const char *path);
int mkdir(const char *path);
int unlink(const char *path);

size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, int c);
void print(const char *str);
void println(const char *str);
void print_num(uint64_t n);
int readline(char *buf, size_t max_len);

#endif
