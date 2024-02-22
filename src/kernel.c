#include <klibc/stdint.h>
#include <klibc/stdlib.h>

#include <drivers/early_screen.h>
#include <drivers/serial.h>
#include <drivers/pit.h>

#include <int/gdt.h>
#include <int/idt.h>
#include <int/isr.h>
#include <sys/apic.h>
#include <sys/pic.h>
#include <sys/smp.h>

#include <io/cpuid.h>

#include <mm/mm.h>
#include <mm/pmm.h>

#include <multiboot.h>

#define print_mem() kprintf("free: %d kb (%d mb), used %d kb (%d mb)\n", pmm_get_free_mem() / 1024, pmm_get_free_mem() / 1024 / 1024, pmm_get_used_mem() / 1024, pmm_get_used_mem() / 1024 / 1024);

void test_handler(int_reg_t* regs, core_locals_t* locals) {
    kprintf("Hello from interrupt! Core %u (in_irq %u)\n", locals->apic_id, locals->in_irq);
}

int32_t kmain(multiboot_t* mboot) {
    screen_init();
    serial_init(COM1, UART_BAUD_115200);

    gdt_init();

    mm_memory_setup(mboot);
    acpi_init(mboot);

    pic_remap(32, 40);
    apic_configure();

    init_core_locals(0);
    idt_init();

    pit_init();
    register_int_handler(0x80, test_handler);

    smp_launch_cpus();

    asm volatile("int $0x80");

    // -- -- -- -- -- -- TEST STUFF -- -- -- -- -- --
    core_locals_t* locals = get_core_locals();
    kprintf("Hello from kmain! Core %u (in_irq %u)\n", locals->apic_id, locals->in_irq);

    print_mem()
    void* ptr1 = kmalloc(0x1000000);
    kfree(ptr1);
    print_mem()

    CPUID_String_t str = {0};
    uint32_t cpuid_max = cpuid_vendor(str);

    sprintf("CPU Vendor: %s\n", str);
    
    cpuid_string(CPUID_INTELBRANDSTRING, str);
    sprintf("CPU BrandString: %s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGMORE, str);
    sprintf("%s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGEND, str);
    sprintf("%s\n", str);

    sleep_no_task(1000);
    sprintf("DONE!\n");

    for (;;);
    return 0xDEADBABA;
}