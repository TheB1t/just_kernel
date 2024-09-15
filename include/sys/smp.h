#pragma once

#include <klibc/stdlib.h>
#include <sys/core.h>

typedef struct {
    uint8_t     status;
    uint32_t    cr3;
    uint32_t    esp;
    uint32_t    entry;
    uint32_t    gdt_ptr;
} __attribute__((packed)) SMPBootInfo_t;

void            init_core_locals_bsp();
void            init_core_locals();
core_locals_t*  get_core_locals();

void            smp_send_global_interrupt(uint8_t int_num);
uint8_t         smp_get_lapic_id();
uint8_t         smp_launch_cpus();