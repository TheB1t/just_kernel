#include <klibc/stdint.h>
#include <klibc/stdlib.h>

#include <drivers/early_screen.h>
#include <drivers/serial.h>
#include <drivers/pit.h>

#include <int/dt.h>
#include <int/isr.h>
#include <sys/pic.h>

#include <mm/mm.h>
#include <mm/pmm.h>

#include <multiboot.h>

#define print_mem() sprintf("free: %d kb, used %d kb\n", pmm_get_free_mem() / 1024, pmm_get_used_mem() / 1024);

void test_handler(int_reg_t* regs) {
    sprintf("Hello from interrupt!\n");
}

int32_t kmain(multiboot_t* mboot) {
    screen_init();
    serial_init(COM1, UART_BAUD_9600);

    pic_remap(32, 40);

    dt_init();

    mm_memory_setup(mboot);
    pit_init();

    kprintf("[early_screen] Hello from kernel!\n");
    sprintf("[serial] Hello from kernel!\n");

    register_int_handler(0x80, test_handler);

    asm volatile("int $0x80");

    print_mem();
    char* c = (char*)kmalloc(100 * 1024 * 1024);
    print_mem();
    kfree(c);
    print_mem();

    sleep_no_task(2000);
    sprintf("DONE!\n");

    for (;;);
    return 0xDEADBABA;
}