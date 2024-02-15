#include <int/dt.h>
#include <int/gdt.h>
#include <int/idt.h>
#include <int/isr.h>

uint32_t dt_init() {
	gdt_init();
	idt_init();

	return 0;
}