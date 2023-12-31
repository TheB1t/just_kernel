#include <int/dt.h>
#include <int/gdt.h>
#include <int/idt.h>
#include <int/isr.h>

void dt_init() {
	gdt_init();
	idt_init();
}