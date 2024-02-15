#include <klibc/stdint.h>
#include <klibc/stdlib.h>

#include <drivers/early_screen.h>
#include <drivers/serial.h>
#include <drivers/pit.h>

#include <int/dt.h>
#include <int/isr.h>
#include <sys/apic.h>
#include <sys/pic.h>

#include <io/cpuid.h>

#include <mm/mm.h>
#include <mm/pmm.h>

#include <multiboot.h>

#define print_mem() kprintf("free: %d kb (%d mb), used %d kb (%d mb)\n", pmm_get_free_mem() / 1024, pmm_get_free_mem() / 1024 / 1024, pmm_get_used_mem() / 1024, pmm_get_used_mem() / 1024 / 1024);

void test_handler(int_reg_t* regs) {
    kprintf("Hello from interrupt!\n");
}

#define INIT(name, check)    {              \
    kprintf("Initializing %s...", name);    \
    kprintf("%s\n", !check ? "OK" : "FAIL"); \
}                                           \

int32_t kmain(multiboot_t* mboot) {
    screen_init();

    INIT("COM1",                serial_init(COM1, UART_BAUD_115200));
    INIT("PIC",                 pic_remap(32, 40));
    INIT("description tables",  dt_init());
    INIT("memory manager",      mm_memory_setup(mboot));
    INIT("ACPI",                acpi_init(mboot));
    INIT("APIC",                apic_configure());
    INIT("PIT",                 pit_init());

    kprintf("[early_screen] Hello from kernel!\n");
    sprintf("[serial] Hello from kernel!\n");

    register_int_handler(0x80, test_handler);

    asm volatile("int $0x80");

    print_mem();
    char* c = (char*)kmalloc(100 * 1024 * 1024);
    print_mem();
    kfree(c);
    print_mem();

    CPUID_String_t str = {0};
    uint32_t cpuid_max = cpuid_vendor(str);

    sprintf("CPU Vendor: %s\n", str);
    
    cpuid_string(CPUID_INTELBRANDSTRING, str);
    sprintf("CPU BrandString: %s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGMORE, str);
    sprintf("%s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGEND, str);
    sprintf("%s\n", str);

    sleep_no_task(2000);
    sprintf("DONE!\n");

    for (;;);
    return 0xDEADBABA;
}