#include "tss.h"

#include "../lib/memory.h"

tss_t kernel_tss;

void tss_init(void)
{
    k_memset(&kernel_tss, 0, sizeof(kernel_tss));
    kernel_tss.iomap_base = sizeof(kernel_tss);
}

void tss_set_rsp0(uint64_t rsp0)
{
    kernel_tss.rsp0 = rsp0;
}