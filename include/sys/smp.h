#pragma once

#include <klibc/stdlib.h>
#include <int/idt.h>

typedef struct {
    uint8_t     status;
    uint8_t     id;
    uint32_t    cr3;
    uint32_t    esp;
    uint32_t    entry;
    uint32_t    gdt_ptr;
} __attribute__((packed)) SMPBootInfo_t;

typedef struct {
    uint32_t    meta_pointer;
    uint32_t    in_irq;

    uint8_t     apic_id;
    uint8_t     core_index;

    idt_entry_t idt_entries[IDT_ENTRIES];
} __attribute__((packed)) core_locals_t;

void            init_core_locals(uint8_t id);
core_locals_t*  get_core_locals();

uint8_t         smp_get_lapic_id();
void            smp_launch_cpus();