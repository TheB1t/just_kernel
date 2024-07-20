#pragma once

#include <klibc/stdlib.h>

#define PL_RING0 0b00
#define PL_RING1 0b01
#define PL_RING2 0b10
#define PL_RING3 0b11

#define DESC_KERNEL_CODE	1
#define DESC_KERNEL_DATA	2
#define DESC_USER_CODE		3
#define DESC_USER_DATA		4
#define DESC_TSS            5

#define DESC_SEG(n, cpl)	(uint32_t)(((uint32_t)0x8 * (uint32_t)n) | (uint32_t)cpl)