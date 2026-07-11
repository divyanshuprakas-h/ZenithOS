#include "apic.h"

#include <stdint.h>

enum
{
    IA32_APIC_BASE_MSR = 0x1Bu,
    IA32_APIC_BASE_ENABLE = 1ull << 11,
    IA32_APIC_BASE_X2APIC = 1ull << 10
};

static uint64_t apic_read_msr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

    return ((uint64_t)high << 32) | low;
}

static void apic_write_msr(uint32_t msr, uint64_t value)
{
    uint32_t low = (uint32_t)(value & 0xFFFFFFFFu);
    uint32_t high = (uint32_t)(value >> 32);

    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

void apic_disable(void)
{
    uint64_t apic_base = apic_read_msr(IA32_APIC_BASE_MSR);
    apic_base &= ~(IA32_APIC_BASE_ENABLE | IA32_APIC_BASE_X2APIC);
    apic_write_msr(IA32_APIC_BASE_MSR, apic_base);
}
