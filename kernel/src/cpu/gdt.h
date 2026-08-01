#ifndef GDT_H
#define GDT_H

#define KERNEL_CODE_SELECTOR    0x08
#define KERNEL_DATA_SELECTOR    0x10

#define USER_CODE_SELECTOR      0x18
#define USER_DATA_SELECTOR      0x20

#define TSS_SELECTR             0x28

void gdt_init(void);
void gdt_load(void *gdtr);

#endif