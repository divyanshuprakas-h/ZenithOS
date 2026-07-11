#ifndef PIC_H
#define PIC_H

#include <stdbool.h>
#include <stdint.h>

#include "ports.h"

#define PIC_PRIMARY_VECTOR 32u
#define PIC_SECONDARY_VECTOR 40u

#define PIC_ICW1_ICW4 0x01u
#define PIC_ICW1_INIT 0x10u
#define PIC_ICW4_8086 0x01u
#define PIC_EOI 0x20u

void pic_init(void);
void pic_remap(uint8_t master_offset, uint8_t slave_offset);
void pic_mask_all(void);
void pic_mask_irq(uint8_t irq_line);
void pic_unmask_irq(uint8_t irq_line);
void pic_send_eoi(uint8_t irq_line);

#endif
