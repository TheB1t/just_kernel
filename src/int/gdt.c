#include <int/gdt.h>

#define shift(v, n)							((v) << (n))
#define pack_access(A, RW, DC, E, DPL, P)	(0x00 | shift(A & 0b1, 0) | shift(RW & 0b1, 1) | shift(DC & 0b1, 2) | shift(E & 0b1, 3) | shift(0b1, 4) | shift(DPL & 0b11, 5) | shift(P & 0b1, 7))
#define pack_access_system(T, DPL, P)		(0x00 | shift(T & 0b1111, 0) | shift(DPL & 0b11, 5) | shift(P & 0b1, 7))
#define pack_flags(A, DB, G)				(0x00 | shift(A & 0b1, 0) | shift(DB & 0b1, 2) | shift(G & 0b1, 3))

gdt_entry_t	gdt_entries[GDT_ENTRIES];
gdt_ptr_t	gdt;

extern void gdt_flush(uint32_t);

void gdt_set_gate_base(int32_t num, uint32_t base) {
	gdt_entries[num].base_low		= (base & 0xFFFF);
	gdt_entries[num].base_middle	= (base >> 16) & 0xFF;
	gdt_entries[num].base_high		= (base >> 24) & 0xFF;
}

void gdt_set_gate_limit(int32_t num, uint32_t limit) {
	gdt_entries[num].limit_low		= (limit & 0xFFFF);
	gdt_entries[num].limit_high		= (limit >> 16) & 0x0F;
}

void gdt_set_gate_flags(int32_t num, uint8_t flags) {
	gdt_entries[num].flags			= flags;
}

void gdt_set_gate_access(int32_t num, uint8_t access) {
	gdt_entries[num].access			= access;
}

void gdt_set_gate_type(int32_t num, uint8_t type) {
	gdt_entries[num].access			&= ~0x0F;
	gdt_entries[num].access			|= (type & 0x0F);
}

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
	gdt_set_gate_base(num, base);
	gdt_set_gate_limit(num, limit);
	gdt_set_gate_flags(num, flags);
	gdt_set_gate_access(num, access);
}

void gdt_init() {
	gdt.limit 	= (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
	gdt.base	= (uint32_t)&gdt_entries;

	gdt_set_gate(0					, 0, 0, 0, 0);																					//Null segment			(0x00)
	gdt_set_gate(DESC_KERNEL_CODE	, 0, 0xFFFFFFFF, pack_access(0b0, 0b1, 0b0, 0b1, PL_RING0, 0b1)	, pack_flags(0b0, 0b1, 0b1));	//Kernel Code segment	(0x08)
	gdt_set_gate(DESC_KERNEL_DATA	, 0, 0xFFFFFFFF, pack_access(0b0, 0b1, 0b0, 0b0, PL_RING0, 0b1)	, pack_flags(0b0, 0b1, 0b1));	//Kernel Data segment	(0x10)
	gdt_set_gate(DESC_USER_CODE		, 0, 0xFFFFFFFF, pack_access(0b0, 0b1, 0b0, 0b1, PL_RING3, 0b1)	, pack_flags(0b0, 0b1, 0b1));	//User Code segment		(0x18)
	gdt_set_gate(DESC_USER_DATA		, 0, 0xFFFFFFFF, pack_access(0b0, 0b1, 0b0, 0b0, PL_RING3, 0b1)	, pack_flags(0b0, 0b1, 0b1));	//User Data segment 	(0x20)
	gdt_set_gate(DESC_TSS			, 0,	   0x68, pack_access_system(0x9, PL_RING3, 0b1)			, pack_flags(0b0, 0b1, 0b1));	//TSS segment 			(0x28)

	gdt_flush((uint32_t)&gdt);
}