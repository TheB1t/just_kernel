#pragma once

#include <klibc/stdint.h>

extern uint64_t    read_msr(uint32_t msr);
extern void        write_msr(uint32_t msr, uint64_t data);
extern uint64_t    read_tsc();