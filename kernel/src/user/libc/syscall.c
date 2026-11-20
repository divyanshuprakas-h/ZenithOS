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

int sysinfo(sysinfo_t *info)
{
    return (int)syscall1(SYS_SYSINFO, (long)info);
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s && s[len])
    {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (s1[i] != s2[i])
        {
            return (int)(unsigned char)s1[i] - (int)(unsigned char)s2[i];
        }
        if (s1[i] == '\0')
        {
            return 0;
        }
    }
    return 0;
}

void print(const char *str)
{
    if (str != NULL)
    {
        write(1, str, (uint64_t)strlen(str));
    }
}

void println(const char *str)
{
    print(str);
    write(1, "\n", 1);
}

void print_num(uint64_t n)
{
    if (n == 0)
    {
        write(1, "0", 1);
        return;
    }

    char buf[32];
    int i = 0;
    while (n > 0)
    {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }

    for (int j = 0; j < i / 2; j++)
    {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }

    write(1, buf, (uint64_t)i);
}

int readline(char *buf, size_t max_len)
{
    if (buf == NULL || max_len == 0)
    {
        return 0;
    }

    size_t count = 0;
    while (count + 1 < max_len)
    {
        char c;
        int n = read(0, &c, 1);
        if (n <= 0)
        {
            yield();
            continue;
        }

        if (c == '\n' || c == '\r')
        {
            break;
        }

        buf[count++] = c;
    }

    buf[count] = '\0';
    return (int)count;
}
