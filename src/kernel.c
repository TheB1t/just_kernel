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
#include <sys/tss.h>
#include <sys/stack_trace.h>

#include <proc/syscalls.h>
#include <proc/sched.h>

#include <io/cpuid.h>

#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

#include <multiboot.h>

/*
    TODO:
    - Implement VFS
    - Split kernel and user space memory
    - Use HPET instead of PIT
    - initrd support
*/

multiboot_info_t mboot_s = {0};

void test_handler() {
    core_locals_t* locals = get_core_locals();
    ser_printf("Hello from syscall! (core %u, %s, tid %u, in_irq %u, cs 0x%02x)\n", locals->core_id, locals->current_thread->parent->name, locals->current_thread->tid, locals->in_irq, locals->irq_regs->cs);
}

void test_thread() {
    syscall0(100);
    syscall1(0, 0xDEADBEEF);
    UNREACHEBLE;
}

void kernel_thread() {
    // pmm_print_memory_stats();

    process_t* proc = proc_create_kernel("test0", 0);
    thread_create(proc, test_thread);
    thread_create(proc, test_thread);
    thread_create(proc, test_thread);
    sched_run_proc(proc);

    proc = proc_create_kernel("test1", 3);
    thread_create(proc, test_thread);
    thread_create(proc, test_thread);
    thread_create(proc, test_thread);
    sched_run_proc(proc);

    // -- -- -- -- -- -- TEST STUFF -- -- -- -- -- --
    core_locals_t* locals = get_core_locals();
    kprintf("Hello from kmain! Core %u (in_irq %u)\n", locals->core_id, locals->in_irq);

    CPUID_String_t str = {0};
    cpuid_vendor(str);

    kprintf("CPU Vendor: %s\n", str);

    cpuid_string(CPUID_INTELBRANDSTRING, str);
    kprintf("CPU BrandString: %s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGMORE, str);
    kprintf("%s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGEND, str);
    kprintf("%s\n", str);

    kprintf("Wait 1 second!\n");
    syscall1(1, 1000);
    kprintf("DONE!\n");

    // kprintf("Hello from kmain! Core %u (in_irq %u)\n", locals->apic_id, locals->in_irq);
    // pmm_print_memory_stats();

    // uint32_t* hello_page_fault = (uint32_t*)0xDEADBEEF;
    // *hello_page_fault = 0xFACEB00C;

    syscall1(0, 0xDEADBEEF);
    UNREACHEBLE;
}

void kmain(multiboot_info_t* mboot) {
    memcpy((uint8_t*)mboot, (uint8_t*)&mboot_s, sizeof(multiboot_info_t));

    gdt_init();
    init_core_locals_bsp();

    screen_init();
    serial_init(COM1, UART_BAUD_115200);

    idt_init();
    tss_init();

    acpi_init();
    mm_memory_setup(&mboot_s);

    pic_remap(32, 40);
    apic_configure();

	ser_printf("[GRUB] Loaded %d modules\n", mboot_s.mods_count);
	ser_printf("[GRUB] Loaded %s table\n",
		mboot_s.flags & MULTIBOOT_INFO_AOUT_SYMS    ? "symbol" :
		mboot_s.flags & MULTIBOOT_INFO_ELF_SHDR     ? "section" : "no one"
	);

	if (mboot_s.flags & MULTIBOOT_INFO_ELF_SHDR) {
		init_kernel_table((void*)mboot_s.u.elf_sec.addr, mboot_s.u.elf_sec.size, mboot_s.u.elf_sec.num, mboot_s.u.elf_sec.shndx);
	} else {
		ser_printf("Section table isn't loaded! Stacktrace in semi-functional mode");
	}

    pit_init();

    sched_init(kernel_thread);

    syscalls_init();
    syscalls_set(100, (void*)test_handler);

    asm volatile("sti");

    uint8_t booted = smp_launch_cpus();
    kprintf("Booted %u cores\n", booted);

    sched_init_core();
    sched_run();
    UNREACHEBLE;
}