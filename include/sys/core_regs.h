#pragma once

#include <klibc/stdlib.h>

typedef struct {
    uint32_t    cr3;

    uint32_t    ds;

    uint32_t    edi;
    uint32_t    esi;
    uint32_t    ebp;
    uint32_t    esp;
    uint32_t    ebx;
    uint32_t    edx;
    uint32_t    ecx;
    uint32_t    eax;

    uint32_t    int_num;
    union {
        uint32_t    int_err;
        struct {
            uint32_t    external        : 1;
            uint32_t    table_indicator : 2;
            uint32_t    selector        : 29;
        } gpf_err;
    };

    uint32_t    eip;
    uint32_t    cs;
    uint32_t    eflags;

    uint32_t    user_esp;
    uint32_t    user_ss;
} __attribute__((packed)) core_regs_t;

typedef uint8_t __attribute__((aligned(16))) sse_region_t[512];