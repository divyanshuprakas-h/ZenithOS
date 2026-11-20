#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>

#define SYS_EXIT   0
#define SYS_OPEN   1
#define SYS_READ   2
#define SYS_WRITE  3
#define SYS_CLOSE  4
#define SYS_YIELD  5

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

#endif
