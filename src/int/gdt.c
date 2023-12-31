#include <int/gdt.h>

gdt_entry_t	gdt_entries[GDT_ENTRIES];
gdt_ptr_t	gdt;

extern void gdt_flush(uint32_t);

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity) {
	gdt_entries[num].base_low		= (base & 0xFFFF);
	gdt_entries[num].base_middle	= (base >> 16) & 0xFF;
	gdt_entries[num].base_high		= (base >> 24) & 0xFF;

	gdt_entries[num].limit_low		= (limit & 0xFFFF);
	gdt_entries[num].granularity	= (limit >> 16) & 0x0F;

	gdt_entries[num].granularity	|= granularity & 0xF0;
	gdt_entries[num].access			= access;
}

void gdt_init() {
	gdt.limit 	= (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
	gdt.base	= (uint32_t)&gdt_entries;

	gdt_set_gate(0					, 0, 0, 0, 0);																				//Null segment			(0x00)
	gdt_set_gate(DESC_KERNEL_CODE	, 0, 0xFFFFFFFF, pack_access(0b1010, 0b1, PL_RING0, 0b1), pack_granularity(0b0, 0b1, 0b1)); //Kernel Code segment	(0x08)
	gdt_set_gate(DESC_KERNEL_DATA	, 0, 0xFFFFFFFF, pack_access(0b0010, 0b1, PL_RING0, 0b1), pack_granularity(0b0, 0b1, 0b1));	//Kernel Data segment	(0x10)
	gdt_set_gate(DESC_USER_CODE	    , 0, 0xFFFFFFFF, pack_access(0b1010, 0b1, PL_RING3, 0b1), pack_granularity(0b0, 0b1, 0b1)); //User Code segment		(0x18)
	gdt_set_gate(DESC_USER_DATA	    , 0, 0xFFFFFFFF, pack_access(0b0010, 0b1, PL_RING3, 0b1), pack_granularity(0b0, 0b1, 0b1)); //User Data segment 	(0x20)

	gdt_flush((uint32_t)&gdt);
}