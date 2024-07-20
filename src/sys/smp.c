#include <drivers/pit.h>
#include <sys/smp.h>
#include <sys/apic.h>

#include <sys/tss.h>
#include <int/gdt.h>
#include <int/idt.h>
#include <io/msr.h>
#include <mm/vmm.h>
#include <proc/sched.h>

SMPBootInfo_t* smp_info_ptr = (SMPBootInfo_t*)0x500;
uint8_t smp_boot_counter    = 1;

extern gdt_ptr_t gdt;
extern char smp_trampoline[];
extern char smp_trampoline_end[];
extern char smp_loaded[];

void init_core_locals(uint8_t id) {
    core_locals_t* locals   = kcalloc(sizeof(core_locals_t));
    memset((uint8_t*)locals, 0, sizeof(core_locals_t));
    locals->state           = CORE_OFFLINE;
    locals->meta_pointer    = (uint32_t)locals;
    locals->irq_stack       = (uint32_t)(kmalloc(PAGE_SIZE) + PAGE_SIZE);
    locals->apic_id         = smp_get_lapic_id();
    locals->core_index      = id;
    write_msr(0xC0000101, (uint32_t)locals);
}

core_locals_t* get_core_locals() {
    core_locals_t* ret;
    asm volatile("mov %%gs:(0), %0;" : "=r"(ret));
    return ret;
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

void smp_launch_cpus() {
    get_core_locals()->state = CORE_ONLINE;

    uint32_t code_size = smp_trampoline_end - smp_trampoline;

    if (code_size > PAGE_SIZE) {
        sprintf("[SMP] Trampoline code size is too big (> 4096 bytes)");
    }

    vmm_remap((void*)0, (void*)0, 2, VMM_WRITE | VMM_PRESENT);

    memcpy((uint8_t*)smp_trampoline, (uint8_t*)0x1000, code_size);
    memset((uint8_t*)smp_info_ptr, 0, sizeof(SMPBootInfo_t));
    memcpy((uint8_t*)&gdt, (uint8_t*)0x600, sizeof(gdt_ptr_t));

    sprintf("[SMP] Start initialization\n");
    uint32_t id = 0;
    struct list_head* iter0;
    list_for_each(iter0, LIST_GET_HEAD(&madt_cpu)) {
        madt_ent0_t* cpu = (madt_ent0_t*)plist_get(iter0);

        if (cpu->apic_id != smp_get_lapic_id()) {
            if (cpu->cpu_flags & 0b11) {

                memset((uint8_t*)smp_info_ptr, 0, sizeof(SMPBootInfo_t));

                smp_info_ptr->status    = 0;
                smp_info_ptr->id        = id;
                smp_info_ptr->cr3       = (uint32_t)vmm_get_base();
                smp_info_ptr->esp       = (uint32_t)(kmalloc(0x4000) + 0x4000);
                smp_info_ptr->entry     = (uint32_t)smp_loaded;
                smp_info_ptr->gdt_ptr   = 0x600;

                for (uint32_t try = 0; try < 2; try++) {
                    smp_send_ipi(cpu->apic_id, 0x500);
                    sleep_no_task(10);
                    smp_send_ipi(cpu->apic_id, 0x600 | 0x1);
                    sleep_no_task(try == 0 ? 10 : 1000);

                    if (smp_info_ptr->status > 0) {
                        if (smp_info_ptr->status == 1) {
                            sprintf("[SMP] Core %u stuck on boot! Waiting...\n", id);
                            sleep_no_task(1000);
                        }

                        if (smp_info_ptr->status == 2) {
                            sprintf("[SMP] Core %u on configuration step! Waiting...\n", id);
                            sleep_no_task(1000);
                        }

                        if (smp_info_ptr->status == 3) {
                            sprintf("[SMP] Core %u booted!\n", id);
                        } else {
                            sprintf("[SMP] Core %u failed to start.\n", id);
                        }
                        break;
                    }
                }
            }
        } else {
            sprintf("[SMP] Core %u is BSP.\n", id);
        };

        id++;
    }

    sprintf("[SMP] End initialization, %u cores booted\n", smp_boot_counter);

    vmm_unmap((void*)0, 2);
}

void smp_entry_point(SMPBootInfo_t* info) {
    apic_configure_ap();

    init_core_locals(info->id);
    sched_init_core();
    tss_init();
    idt_init();

    asm volatile("int $0x80");

    info->status = 3;

    get_core_locals()->state = CORE_ONLINE;

    while (1) {
        asm volatile("hlt");
    }
}