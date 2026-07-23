#include "tests.h"

#include "../terminal/terminal.h"

#include "../hal/apic.h"
#include "../hal/pic.h"

#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"

void driver_tests(void)
{
    terminal_write("\n------------ DRIVER TESTS ------------\n");

    apic_disable();
    terminal_write("APIC OK\n");

    pic_init();
    terminal_write("PIC OK\n");

    keyboard_init();
    terminal_write("Keyboard OK\n");

    mouse_init();
    terminal_write("Mouse OK\n");

    terminal_write("DRIVER TESTS PASSED\n");
}