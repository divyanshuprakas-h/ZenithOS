#ifndef GDT_H
#define GDT_H

void gdt_init(void);
void gdt_load(void *gdtr);

#endif