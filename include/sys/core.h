#pragma once

#include <klibc/stdlib.h>
#include <int/idt.h>
#include <sys/tss.h>
#include <proc/sched.h>

typedef struct {
    uint32_t    esp;
    uint32_t    ss;
    uint32_t    ds;
    uint32_t    cr3;
    uint32_t    esi;
    uint32_t    edi;
    uint32_t    ebp;
    uint32_t    edx;
    uint32_t    ecx;
    uint32_t    ebx;
    uint32_t    eax;

    uint32_t    int_num;
    uint32_t    int_err;

    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;

    uint32_t    esp0;
    uint32_t    ss0;
} __attribute__((packed)) core_regs_t;

typedef enum {
    CORE_OFFLINE,
    CORE_ONLINE,
    CORE_IDLE,

    _CORE_LAST,
    CORE_MAX = _CORE_LAST
} core_state_t;

typedef struct {
    uint32_t    meta_pointer;
    uint32_t    in_irq;
    uint32_t    irq_stack;

    core_regs_t irq_regs;
    core_state_t state;

    thread_t*   current_thread;
    thread_t*   idle_thread;

    uint8_t     apic_id;
    uint8_t     core_index;

    idt_entry_t idt_entries[IDT_ENTRIES];
    tss_entry_t tss_entry;
} __attribute__((packed)) core_locals_t;