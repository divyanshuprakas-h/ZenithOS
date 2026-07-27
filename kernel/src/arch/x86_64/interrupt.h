#ifndef ARC_X86_64_INTERRUPT_H
#define ARC_X86_64_INTERRUPT_H

#include <stdbool.h>

void cpu_cli(void);
void cpu_sti(void);
bool cpu_interrupts_enabled(void);

#endif