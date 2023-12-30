#include <klibc/stdint.h>
#include <klibc/stdio.h>
#include <drivers/early_screen.h>
#include <multiboot.h>

int32_t kmain(multiboot_t* mboot) {
    screen_init();

    kprintf("Hello from kernel!\n");

    for (;;);
    return 0xDEADBABA;
}