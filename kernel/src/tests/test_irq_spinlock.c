#include "../sync/irq_spinlock.h"
#include "../stdio/printf.h"

void test_irq_spinlock(void)
{
    irq_spinlock_t lock;

    irq_spinlock_init(&lock);

    irq_spin_lock(&lock);

    if (lock.lock.locked == 1)
    {
        kprintf("[PASS] irq spinlock acquired\n");
    }
    else
    {
        kprintf("[FAIL] irq spinlock acquire\n");
    }

    irq_spin_unlock(&lock);

    if (lock.lock.locked == 0)
    {
        kprintf("[PASS] irq spinlock released\n");
    }
    else
    {
        kprintf("[FAIL] irq spinlock release\n");
    }
}