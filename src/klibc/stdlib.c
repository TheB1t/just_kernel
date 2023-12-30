#include <klibc/stdlib.h>
#include <klibc/stdio.h>

void panic(char* msg) {
    kprintf("Kernel PANIC!!! [%s]\n", msg);

    while (1)
        asm volatile("hlt");
}