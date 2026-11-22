#include "tests.h"

#include "../terminal/terminal.h"
#include "../drivers/serial.h"

void driver_tests(void)
{
    terminal_write("  [TEST] Running Driver tests...\n");

    // COM1 Serial driver assertions
    TEST_ASSERT(serial_is_present());

    serial_status_t status;
    serial_get_status(&status);
    TEST_ASSERT(status.is_online);
    TEST_ASSERT(status.port == COM1_PORT);
    TEST_ASSERT(status.baud_rate == 115200);
}
