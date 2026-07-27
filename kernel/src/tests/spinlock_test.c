#include "../sync/spinlock.h"
#include "../stdio/printf.h"

void test_spinlock(void)
{
    spinlock_t lock;

    spinlock_init(&lock);
    
    kprintf("Spinlock Test\n");

    spin_lock(&lock);

    if (lock.locked == 1)
    {
        kprintf("[PASS] lock acquired\n");
    }
    else
    {
        kprintf("[FAIL] lock acquire\n");
    }

    spin_unlock(&lock);

    if (lock.locked == 0)
    {
        kprintf("[PASS] lock released\n");
    }
    else
    {
        kprintf("[FAIL] lock release\n");
    }
}