#pragma once

#include <klibc/stdlib.h>
#include <int/dt.h>

#define IDT_ENTRIES 256

typedef struct {
	uint16_t	base_low;
	uint16_t	sel;
	uint8_t		always0;
	uint8_t		flags;
	uint16_t	base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed)) idt_ptr_t;

void idt_init();