#include "syscall.h"

long syscall0(long number)
{
    long ret;
    register long r_num __asm__("rdi") = number;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "r"(r_num)
        : "memory"
    );
    return ret;
}

long syscall1(long number, long arg1)
{
    long ret;
    register long r_num __asm__("rdi") = number;
    register long r_a1  __asm__("rsi") = arg1;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "r"(r_num), "r"(r_a1)
        : "memory"
    );
    return ret;
}

long syscall2(long number, long arg1, long arg2)
{
    long ret;
    register long r_num __asm__("rdi") = number;
    register long r_a1  __asm__("rsi") = arg1;
    register long r_a2  __asm__("rdx") = arg2;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "r"(r_num), "r"(r_a1), "r"(r_a2)
        : "memory"
    );
    return ret;
}

long syscall3(long number, long arg1, long arg2, long arg3)
{
    long ret;
    register long r_num __asm__("rdi") = number;
    register long r_a1  __asm__("rsi") = arg1;
    register long r_a2  __asm__("rdx") = arg2;
    register long r_a3  __asm__("rcx") = arg3;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "r"(r_num), "r"(r_a1), "r"(r_a2), "r"(r_a3)
        : "memory"
    );
    return ret;
}

int open(const char *path)
{
    return (int)syscall1(SYS_OPEN, (long)path);
}

int read(int fd, void *buf, uint64_t count)
{
    return (int)syscall3(SYS_READ, (long)fd, (long)buf, (long)count);
}

int write(int fd, const void *buf, uint64_t count)
{
    return (int)syscall3(SYS_WRITE, (long)fd, (long)buf, (long)count);
}

int close(int fd)
{
    return (int)syscall1(SYS_CLOSE, (long)fd);
}

void exit(int status)
{
    syscall1(SYS_EXIT, (long)status);
    while (1)
    {
        __asm__ volatile("pause");
    }
}

void yield(void)
{
    syscall0(SYS_YIELD);
}
