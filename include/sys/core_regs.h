#pragma once

#include <klibc/stdlib.h>

typedef struct {
    uint32_t    esp;
    uint32_t    ss;
} __attribute__((packed)) core_regs_user_t;

typedef struct {
    uint32_t    es;
    uint32_t    ds;
    uint32_t    fs;
    uint32_t    gs;
} __attribute__((packed)) core_regs_v86_t;

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
        } err;
    };

    uint32_t    eip;
    uint32_t    cs;

    union {
        uint32_t    eflags;
        struct {
            uint32_t    carry_flag  : 1;
            uint32_t    res0        : 1;
            uint32_t    parity_flag : 1;
            uint32_t    res1        : 1;
            uint32_t    acarry_flag : 1;
            uint32_t    res3        : 1;
            uint32_t    zero_flag   : 1;
            uint32_t    sign_flag   : 1;
            uint32_t    trap_flag   : 1;
            uint32_t    int_en_flag : 1;
            uint32_t    dir_flag    : 1;
            uint32_t    overf_flag  : 1;
            uint32_t    iopl        : 2;
            uint32_t    nested_task : 1;
            uint32_t    res4        : 1;
            uint32_t    resume_flag : 1;
            uint32_t    v86_mode    : 1;
            uint32_t    align_check : 1;
            uint32_t    vint_flag   : 1;
            uint32_t    vint_pend   : 1;
            uint32_t    id_flag     : 1;
            uint32_t    res5        : 10;
        };
    };
} __attribute__((packed)) core_regs_base_t;

typedef struct {
    core_regs_base_t    base;
    core_regs_user_t    user;
    core_regs_v86_t     v86;
} core_regs_t;

typedef uint8_t __attribute__((aligned(16))) sse_region_t[512];