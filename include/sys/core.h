#pragma once

#include <klibc/stdlib.h>
#include <int/idt.h>
#include <sys/core_regs.h>
#include <sys/tss.h>
#include <proc/sched.h>
#include <fs/elf/elf.h>

typedef enum {
    CORE_OFFLINE,
    CORE_BOOTING,
    CORE_ONLINE,
    CORE_IDLE,

    _CORE_LAST,
    CORE_MAX = _CORE_LAST
} core_state_t;

typedef struct {
    union {
        uint8_t     flags;
        struct {
            uint8_t     in_irq          : 1;
            uint8_t     nested_irq      : 1;
            uint8_t     need_resched    : 1;
            uint8_t     reserved        : 5;
        };
    };

    core_regs_t* irq_regs;
    core_state_t state;

    thread_t*   current_thread;
    thread_t*   idle_thread;

    uint8_t     core_id;

    idt_ptr_t   idt_entries_ptr;
    idt_entry_t idt_entries[IDT_ENTRIES];
    tss_entry_t tss_entry;
} __attribute__((packed)) core_locals_t;