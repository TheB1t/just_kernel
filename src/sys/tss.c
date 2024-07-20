#include <sys/tss.h>
#include <int/gdt.h>
#include <sys/smp.h>

extern void tss_flush(uint32_t);
extern void gdt_set_gate_base(int32_t num, uint32_t base);
extern void gdt_set_gate_type(int32_t num, uint8_t type);

void tss_init() {
	core_locals_t* locals = get_core_locals();
	tss_entry_t* tss_entry = &locals->tss_entry;

	// tss_entry->ss0	= DESC_SEG(DESC_KERNEL_DATA, PL_RING3);
	tss_entry->esp0	= (uint32_t)locals->irq_stack;

	/*
		I think this is bad practice to use same TSS for each core.
		Needs to be reworked in the future.
	*/
	gdt_set_gate_base(DESC_TSS, (uint32_t)tss_entry);
	gdt_set_gate_type(DESC_TSS, 0x9);
	tss_flush(DESC_SEG(DESC_TSS, PL_RING0));
}

void set_kernel_stack(uint32_t stack) {
	get_core_locals()->tss_entry.esp0 = stack;
}

uint32_t get_kernel_stack() {
	return get_core_locals()->tss_entry.esp0;
}
