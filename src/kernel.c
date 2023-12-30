#include <klibc/stdint.h>
#include <klibc/stdio.h>
#include <drivers/early_screen.h>
#include <drivers/serial.h>
#include <multiboot.h>

int32_t kmain(multiboot_t* mboot) {
    screen_init();
    serial_init(COM1, UART_BAUD_9600);

    kprintf("[early_screen] Hello from kernel!\n");
    sprintf("[serial] Hello from kernel!\n");

    for (;;);
    return 0xDEADBABA;
}