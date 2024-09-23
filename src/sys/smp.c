#include <drivers/pit.h>
#include <sys/smp.h>
#include <sys/apic.h>

#include <sys/tss.h>
#include <int/gdt.h>
#include <int/idt.h>
#include <io/msr.h>
#include <mm/vmm.h>
#include <proc/sched.h>
#include <proc/syscalls.h>

SMPBootInfo_t* smp_info_ptr = (SMPBootInfo_t*)0x500;
uint8_t smp_boot_counter    = 1;

extern gdt_ptr_t gdt;
extern char smp_trampoline[];
extern char smp_trampoline_end[];
extern char smp_loaded[];

core_locals_t bsp_core_locals = { 0 };
// sse_region_t sse_region = { 0 };

void init_core_locals_bsp() {
    bsp_core_locals.idt_entries_ptr.base  = (uint32_t)bsp_core_locals.idt_entries;
    bsp_core_locals.idt_entries_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;

    // write_msr(0xC0000101, (uint32_t)&bsp_core_locals);
    write_msr(0xC0000102, (uint32_t)&bsp_core_locals);
}

void init_core_locals() {
    core_locals_t* locals = kmalloc(sizeof(core_locals_t));

    memset((uint8_t*)locals, 0, sizeof(core_locals_t));

    locals->state           = CORE_OFFLINE;
    locals->core_id         = smp_get_lapic_id();

    core_locals_t* phys_locals = (core_locals_t*)vmm_virt_to_phys((void*)locals);
    locals->idt_entries_ptr.base  = (uint32_t)phys_locals->idt_entries;
    locals->idt_entries_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;

    // write_msr(0xC0000101, (uint32_t)locals);
    write_msr(0xC0000102, (uint32_t)locals);
}

core_locals_t* get_core_locals() {
    return (core_locals_t*)(uint32_t)read_msr(0xC0000102);
}

uint8_t smp_get_lapic_id() {
    return (read_lapic(0x20) >> 24) & 0xFF;
}

void smp_send_ipi(uint8_t ap, uint32_t ipi_number) {
    write_lapic(0x310, (ap << 24));
    write_lapic(0x300, ipi_number);
}

void smp_send_global_ipi(uint32_t ipi_number) {
    struct list_head* iter0;
    list_for_each(iter0, LIST_GET_HEAD(&madt_cpu)) {
        madt_ent0_t* cpu = (madt_ent0_t*)plist_get(iter0);

        if (cpu->apic_id != smp_get_lapic_id())
            smp_send_ipi(cpu->apic_id, ipi_number);
    }
}

void smp_send_global_interrupt(uint8_t int_num) {
    smp_send_global_ipi((1 << 14) | int_num);
}

uint8_t smp_launch_cpus() {
    uint32_t code_size = smp_trampoline_end - smp_trampoline;

    if (code_size > PAGE_SIZE) {
        ser_printf("[SMP] Trampoline code size is too big (> 4096 bytes)");
    }

    vmm_remap((void*)0, (void*)0, 3, VMM_WRITE | VMM_PRESENT);

    memcpy((uint8_t*)smp_trampoline, (uint8_t*)0x1000, code_size);
    memset((uint8_t*)smp_info_ptr, 0, sizeof(SMPBootInfo_t));
    memcpy((uint8_t*)&gdt, (uint8_t*)0x600, sizeof(gdt_ptr_t));

    ser_printf("[SMP] Start initialization\n");
    uint32_t id = 0;

    struct list_head* iter0;
    list_for_each(iter0, LIST_GET_HEAD(&madt_cpu)) {
        madt_ent0_t* cpu = (madt_ent0_t*)plist_get(iter0);

        if (cpu->apic_id != smp_get_lapic_id()) {
            if (cpu->cpu_flags & 0b11) {

                memset((uint8_t*)smp_info_ptr, 0, sizeof(SMPBootInfo_t));

                smp_info_ptr->status    = 0;
                smp_info_ptr->cr3       = (uint32_t)vmm_get_cr3();
                smp_info_ptr->esp       = (uint32_t)0xA000;
                smp_info_ptr->entry     = (uint32_t)smp_loaded;
                smp_info_ptr->gdt_ptr   = 0x600;

                for (uint32_t try = 0; try < 2; try++) {
                    smp_send_ipi(cpu->apic_id, 0x500);
                    sleep_no_task(100);
                    smp_send_ipi(cpu->apic_id, 0x600 | 0x1);
                    sleep_no_task(try == 0 ? 100 : 1000);

                    if (smp_info_ptr->status > 0) {
                        if (smp_info_ptr->status == 1) {
                            ser_printf("[SMP] Core %u stuck on boot! Waiting...\n", id);
                            sleep_no_task(100);
                        }

                        if (smp_info_ptr->status == 2) {
                            ser_printf("[SMP] Core %u on configuration step! Waiting...\n", id);
                            sleep_no_task(100);
                        }

                        if (smp_info_ptr->status == 3) {
                            ser_printf("[SMP] Core %u booted!\n", id);
                        } else {
                            ser_printf("[SMP] Core %u failed to start.\n", id);
                        }
                        break;
                    }
                }
            }
        } else {
            ser_printf("[SMP] Core %u is BSP.\n", id);
        };

        id++;
    }

    ser_printf("[SMP] End initialization, %u cores booted\n", smp_boot_counter);

    vmm_unmap((void*)0, 3);

    return smp_boot_counter;
}

void smp_entry_point(SMPBootInfo_t* info) {
    apic_configure_ap();

    init_core_locals();
    tss_init();
    idt_init();

    sched_init_core();

    info->status = 3;

    asm volatile("sti");
    syscall0(2);
    while(1)
        asm volatile("hlt");
}