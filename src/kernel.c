#include <klibc/stdint.h>
#include <klibc/stdio.h>

#include <drivers/early_screen.h>
#include <drivers/serial.h>

#include <int/dt.h>
#include <int/isr.h>
#include <sys/pic.h>

#include <multiboot.h>

void test_handler(int_reg_t* regs) {
    sprintf("Hello from interrupt!\n");
}

int32_t kmain(multiboot_t* mboot) {
    screen_init();
    serial_init(COM1, UART_BAUD_9600);

    pic_remap(32, 40);

    dt_init();

    kprintf("[early_screen] Hello from kernel!\n");
    sprintf("[serial] Hello from kernel!\n");

    register_int_handler(0x80, test_handler);

    asm volatile("int $0x80");

    for (;;);
    return 0xDEADBABA;
}