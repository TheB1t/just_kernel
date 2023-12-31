#pragma once

#include <klibc/stdlib.h>
#include <int/dt.h>

#define pack_access(Type, DT, DPL, P) ((((0x00 | (Type & 0b1111)) | (DT & 0b1) << 4) | (DPL & 0b11) << 5) | (P & 0b1) << 7)
#define pack_granularity(A, D, G) (((0x00 | (A & 0b1) << 4) | (D & 0b1) << 6) | (G & 0b1) << 7)

#define GDT_ENTRIES 5

typedef struct {
	uint16_t	limit_low;
	uint16_t	base_low;
	uint8_t		base_middle;
	uint8_t		access;
	uint8_t		granularity;
	uint8_t		base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed)) gdt_ptr_t;

void gdt_init();