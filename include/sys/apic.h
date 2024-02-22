#pragma once

#include <klibc/stdint.h>
#include <sys/acpi/acpi.h>
#include <klibc/plist.h>

#define APIC_BASE_MSR           0x1B
#define APIC_BASE_MSR_ENABLE    0x800
#define REDIRECT_TABLE_BAD_READ 0xFFFFFFFFFFFFFFFF

typedef struct {
    uint8_t     acpi_processor_id;
    uint8_t     apic_id;
    uint32_t    cpu_flags;
} __attribute__((packed)) madt_ent0_t;

typedef struct {
    uint8_t     ioapic_id;
    uint8_t     reserved;
    uint32_t    ioapic_addr;
    uint32_t    gsi_base;
} __attribute__((packed)) madt_ent1_t;

typedef struct {
    uint8_t     bus_src;
    uint8_t     irq_src;
    uint32_t    gsi;
    uint16_t    flags;
} __attribute__((packed)) madt_ent2_t;

typedef struct {
    uint8_t     acpi_processor_id;
    uint16_t    flags;
    uint8_t     lint;
} __attribute__((packed)) madt_ent4_t;

typedef struct {
    uint16_t    reserved;
    uint64_t    local_apic_override;
} __attribute__((packed)) madt_ent5_t;

typedef struct {
    sdt_header_t header;

    uint32_t    local_apic_addr;
    uint32_t    apic_flags;
    uint8_t     entries[];
} __attribute__((packed)) madt_t;

typedef struct {
    uint32_t    addr;
    uint32_t    reserved[3];
    uint32_t    data;
} __attribute__((packed)) ioapic_reg_t;

typedef enum {
    LAPIC_ID    = 0x20,
    LAPIC_VER   = 0x30,
    LAPIC_TPR   = 0x80,
    LAPIC_APR   = 0x90,
    LAPIC_PPR   = 0xA0,
    LAPIC_EOI   = 0xB0,
    LAPIC_RRD   = 0xC0,
    LAPIC_LDR   = 0xD0,
    LAPIC_DFR   = 0xE0,
    LAPIC_SIVR  = 0xF0,

    LAPIC_ISR0  = 0x100,
    LAPIC_ISR1  = 0x110,
    LAPIC_ISR2  = 0x120,
    LAPIC_ISR3  = 0x130,
    LAPIC_ISR4  = 0x140,
    LAPIC_ISR5  = 0x150,
    LAPIC_ISR6  = 0x160,
    LAPIC_ISR7  = 0x170,

    LAPIC_TMR0  = 0x180,
    LAPIC_TMR1  = 0x190,
    LAPIC_TMR2  = 0x1A0,
    LAPIC_TMR3  = 0x1B0,
    LAPIC_TMR4  = 0x1C0,
    LAPIC_TMR5  = 0x1D0,
    LAPIC_TMR6  = 0x1E0,
    LAPIC_TMR7  = 0x1F0,

    LAPIC_IRR0  = 0x200,
    LAPIC_IRR1  = 0x210,
    LAPIC_IRR2  = 0x220,
    LAPIC_IRR3  = 0x230,
    LAPIC_IRR4  = 0x240,
    LAPIC_IRR5  = 0x250,
    LAPIC_IRR6  = 0x260,
    LAPIC_IRR7  = 0x270,

    LAPIC_ESR   = 0x280,

    LAPIC_LVT_CMCI  = 0x2F0,
    LAPIC_ICR0  = 0x300,
    LAPIC_ICR1  = 0x310,
    LAPIC_LVT_TR    = 0x320,
    LAPIC_LVT_TSR   = 0x330,
    LAPIC_LVT_PMCR  = 0x340,
    LAPIC_LVT_LINT0 = 0x350,
    LAPIC_LVT_LINT1 = 0x360,
    LAPIC_LVT_ER    = 0x370,
    LAPIC_ICR       = 0x380,
    LAPIC_CCR       = 0x390,
    LAPIC_DCR       = 0x3E0
} lapic_reg_t;

extern plist_root_t madt_cpu;
extern plist_root_t madt_iso;
extern plist_root_t madt_ioapic;
extern plist_root_t madt_nmi;

extern uint32_t ioapic_read(void* ioapic_base, uint32_t addr);
extern void     ioapic_write(void* ioapic_base, uint32_t addr, uint32_t data);
extern uint32_t read_lapic(lapic_reg_t reg);
extern void     write_lapic(lapic_reg_t reg, uint32_t data);

void        apic_configure_ap();
uint32_t    apic_configure();
void        apic_EOI();