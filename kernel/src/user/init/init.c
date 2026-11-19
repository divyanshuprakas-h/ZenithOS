#include "syscall.h"

int main(void)
{
    static const char msg[] = "\n>>> ZenithOS: Hello from Ring 3 (User Space)! <<<\n\n";
    write(1, msg, sizeof(msg) - 1);

    for (;;)
    {
        yield();
        __asm__ volatile("pause");
    }
}
