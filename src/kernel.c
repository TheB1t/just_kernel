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

#include <proc/sched.h>

#include <io/cpuid.h>

#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

#include <multiboot.h>

multiboot_t mboot_s = {0};

#define print_mem() kprintf("free: %d kb (%d mb), used %d kb (%d mb)\n", pmm_get_free_mem() / 1024, pmm_get_free_mem() / 1024 / 1024, pmm_get_used_mem() / 1024, pmm_get_used_mem() / 1024 / 1024);

void test_handler(core_locals_t* locals) {
    core_regs_t* regs = &locals->irq_regs;

    kprintf("Hello from interrupt! Core %u (in_irq %u)\n", locals->apic_id, locals->in_irq);
    uint32_t esp;	asm volatile("mov %%esp, %0" : "=r" (esp));
    isprintf("IRQ Stack: 0x%08x, Stack: 0x%08x, Stack: 0x%08x, EIP: 0x%08x, SS: 0x%08x\n", locals->irq_stack, esp, regs->esp, regs->eip, regs->ss0);
}

void test_thread() {
    core_locals_t* locals = get_core_locals();

    uint32_t esp;	asm volatile("mov %%esp, %0" : "=r" (esp));
    uint32_t ebp;	asm volatile("mov %%ebp, %0" : "=r" (ebp));
    kprintf("Hello from thread! (core %u, %s, tid %u) | ESP: 0x%08x, EBP: 0x%08x\n", locals->core_index, locals->current_thread->parent->name, locals->current_thread->tid, esp, ebp);
}

void kernel_thread() {
    interrupt_state_t state = interrupt_lock();

    process_t* proc = proc_create("test", test_thread, 0);
    sched_run_proc(proc);

    for (uint32_t i = 0; i < 15; i++) {
        thread_t* t = thread_create(proc, test_thread);
        if (t)
            sched_run_thread(t);
    }

    interrupt_unlock(state);

    // -- -- -- -- -- -- TEST STUFF -- -- -- -- -- --
    core_locals_t* locals = get_core_locals();
    kprintf("Hello from kmain! Core %u (in_irq %u)\n", locals->apic_id, locals->in_irq);

    // print_mem()
    // void* ptr1 = kmalloc(0x1000000);
    // kfree(ptr1);
    // print_mem()

    CPUID_String_t str = {0};
    uint32_t cpuid_max = cpuid_vendor(str);

    sprintf("CPU Vendor: %s\n", str);

    cpuid_string(CPUID_INTELBRANDSTRING, str);
    sprintf("CPU BrandString: %s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGMORE, str);
    sprintf("%s", str);

    cpuid_string(CPUID_INTELBRANDSTRINGEND, str);
    sprintf("%s\n", str);

    kprintf("Wait 1 second!\n");
    sleep_no_task(1000);
    kprintf("DONE!\n");

    for(;;);

    // uint32_t* hello_page_fault = (uint32_t*)0xDEADBEEF;
    // *hello_page_fault = 0xFACEB00C;
}

void kmain(multiboot_t* mboot) {
    memcpy((uint8_t*)mboot, (uint8_t*)&mboot_s, sizeof(multiboot_t));

    screen_init();
    serial_init(COM1, UART_BAUD_115200);

    gdt_init();

    acpi_init(&mboot_s);
    mm_memory_setup(&mboot_s);

    pic_remap(32, 40);
    apic_configure();

    init_core_locals(0);
    tss_init();
    idt_init();

    pit_init();
    register_int_handler(0x80, test_handler);

    sched_init(kernel_thread);

    smp_launch_cpus();

    sched_run();
    for(;;);
}