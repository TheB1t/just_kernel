#include <klibc/stdint.h>
#include <klibc/stdlib.h>

#include <drivers/early_screen.h>
#include <drivers/serial.h>
#include <drivers/pit.h>
#include <drivers/vesa.h>
#include <drivers/char/mem.h>
#include <drivers/char/tty.h>

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
#include <proc/v86.h>

#include <io/cpuid.h>

#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

#include <fs/vfs.h>
#include <fs/devfs.h>
#include <fs/ramdisk/ramdisk.h>
#include <fs/fs.h>
#include <fs/format/bmp.h>

#include <multiboot.h>

/*
    TODO:
    - Implement VFS
    - Split kernel and user space memory
    - Use HPET instead of PIT
    - initrd support
*/

typedef struct {
    uint8_t nographic : 1;
    uint8_t reserved  : 7;
} kernel_flags_t;

kernel_flags_t kernel_flags = {0};
char cmdline[1024] = {0};
multiboot_info_t mboot_s = {0};

void test_handler() {
    core_locals_t* locals = get_core_locals();
    ser_printf("Hello from syscall! (core %u, %s, tid %u, in_irq %u, cs 0x%02x)\n", locals->core_id, locals->current_thread->parent->name, locals->current_thread->tid, locals->in_irq, locals->irq_regs->base.cs);
}

void test_thread() {
    syscall0(100);
    syscall1(0, 0xDEADBEEF);
    UNREACHEBLE;
}

void init() {
    interrupt_state_t state = interrupt_lock();

    (void)vfs_open("/dev/tty0", O_RDWR, 0);     // stdin
    (void)vfs_open("/dev/tty0", O_RDWR, 0);     // stdout
    (void)vfs_open("/dev/tty0", O_RDWR, 0);     // stderr

    char test[] = "Hello from mem reader!\n";
    char test2[] = "SERIAL CONSOLE BABY\n";
    uint8_t* buf[100] = { 0 };

    // Open different files
    int mem = vfs_open("/dev/mem", O_RDWR, 0);
    int pfd = vfs_open("/picture.bmp", O_RDONLY, 0);

    ser_printf("mem: %d, pfd: %d\n", mem, pfd);

    // Memory reading test
    vfs_lseek(mem, (uint32_t)&test, 0);
    vfs_read(mem, (char*)buf, strlen(test) + 1);

    ser_printf("readed: %s", (char*)buf);

    // Memory writing test
    vfs_lseek(mem, (uint32_t)&test, 0);
    vfs_write(mem, (char*)test2, strlen(test2) + 1);

    // Stdout writing test
    vfs_write(1, test, strlen(test));

    // Common reading test
    bmp_draw(0, 200, pfd);

    // for (uint32_t i = 0; i < 100; i++)
    //     kprintf("Hello from kernel! %u\n", i);

    // Cleanup
    vfs_close(mem);
    vfs_close(pfd);

    interrupt_unlock(state);
}

void kernel_thread() {

    mem_init();
    tty_init();
    ramdisk_init();

	if (mboot_s.mods_count > 0) {
        multiboot_module_t* modules = (multiboot_module_t*)mboot_s.mods_addr;

        void* modules_block = (void*)modules[0].mod_start;
        uint32_t modules_block_size = modules[mboot_s.mods_count - 1].mod_end - modules[0].mod_start;

        vmm_map(modules_block, modules_block, modules_block_size / PAGE_SIZE, VMM_PRESENT | VMM_WRITE | VMM_USER);

		for (uint32_t i = 0; i < mboot_s.mods_count; i++) {
            uint32_t* _m = (void*)modules[i].mod_start;
            // uint32_t _m_size = modules[i].mod_end - modules[i].mod_start;

            switch (*_m) {
                case RAMDISK_MAGIC: {
                    ser_printf("Found ramdisk at 0x%x\n", _m);
                    mount_root((void*)_m);
                } break;
            }
        }
	} else {
        panic("Can't load ramdisk!!!");
    }

    vesa_init();
    vesa_switch_to_best_mode(kernel_flags.nographic);

    kprintf("Screen resolution: %dx%d (bpp %d)\n", current_mode->width, current_mode->height, current_mode->bpp);

    interrupt_state_t state = interrupt_lock();

    process_t* proc = proc_create_kernel("test0", 0);
    for (uint32_t i = 0; i < 4; i++)
        thread_create(proc, test_thread);
    sched_run_proc(proc);

    proc = proc_create_kernel("test1", 3);
    for (uint32_t i = 0; i < 4; i++)
        thread_create(proc, test_thread);
    sched_run_proc(proc);

    interrupt_unlock(state);

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

    // uint32_t* hello_page_fault = (uint32_t*)0xDEADBEEF;
    // *hello_page_fault = 0xFACEB00C;

    init();

    kprintf("Wait 1 second!\n");
    syscall1(1, 1000);
    kprintf("DONE!\n");

    syscall1(0, 0xDEADBEEF);
    UNREACHEBLE;
}

void kmain(multiboot_info_t* mboot, uint32_t magic) {
    gdt_init();
    init_core_locals_bsp();

    serial_init(COM1, UART_BAUD_115200);

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("Invalid magic number: %x\n", magic);
        UNREACHEBLE;
    }

    memcpy((uint8_t*)mboot, (uint8_t*)&mboot_s, sizeof(multiboot_info_t));
    uint32_t cmdline_len = strlen((char*)mboot_s.cmdline);

    if (cmdline_len > sizeof(cmdline)) {
        kprintf("Command line too long! (max %d)\n", sizeof(cmdline));
        UNREACHEBLE;
    }

    memcpy((uint8_t*)mboot_s.cmdline, (uint8_t*)cmdline, cmdline_len);

    char* token = strtok((char*)mboot_s.cmdline, " ");
    while (token != NULL) {
        if (strcmp(token, "nographic") == 0) {
            ser_printf("nographic mode enabled!\n");
            kernel_flags.nographic = true;
        }
        token = strtok(NULL, " ");
    }


    idt_init();
    tss_init();

    acpi_init();
    mm_memory_setup(&mboot_s);
    vesa_init_early(&mboot_s);
    if (kernel_flags.nographic)
        screen_init();

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

    v86_init();
    sched_run();
    UNREACHEBLE;
}