#include "../stdio/printf.h"
#include "../arch/x86_64/atomic.h"

void test_atomic_xchg(void)
{
    volatile uint64_t value = 5;

    uint64_t old = atomic_xchg_u64(&value, 10);

    kprintf("Atomic Test\n");

    kprintf("Old Value: %llu\n", old);
    kprintf("New Value: %llu\n",value);

    if (old == 5 && value == 10)
    {
        kprintf("[PASS] atomic_xchg_u64\n");
    }
    else
    {
        kprintf("[FAIL] atomic_xchg_u64\n");
    }
}
