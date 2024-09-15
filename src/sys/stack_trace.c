#include <sys/stack_trace.h>

ELF32Obj_t	kernel_table_obj = { 0 };

void init_kernel_table(void* sybtabPtr, uint32_t size, uint32_t num, uint32_t shindex) {
	vmm_map(sybtabPtr, sybtabPtr, ALIGN(size) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

	KERNEL_TABLE_OBJ->header = NULL;

	KERNEL_TABLE_OBJ->sections.sectab_head			= (ELF32SectionHeader_t*)sybtabPtr;
	KERNEL_TABLE_OBJ->sections.sectab_size			= num;

	KERNEL_TABLE_OBJ->sections.hstrtab_head			= (uint8_t*)ELF32_SECTION(KERNEL_TABLE_OBJ, shindex)->addr;

	ELF32SectionHeader_t* symtab					= ELFLookupSectionByType(KERNEL_TABLE_OBJ, ESHT_SYMTAB);
	KERNEL_TABLE_OBJ->sections.symtab_head			= symtab ? (ELF32Symbol_t*)symtab->addr : NULL;
	KERNEL_TABLE_OBJ->sections.symtab_size 			= symtab ? symtab->size / sizeof(ELF32Symbol_t) : 0;

	ELF32SectionHeader_t* strtab					= ELFLookupSectionByType(KERNEL_TABLE_OBJ, ESHT_STRTAB);
	KERNEL_TABLE_OBJ->sections.strtab_head			= strtab ? (uint8_t*)strtab->addr : NULL;
	KERNEL_TABLE_OBJ->sections.strtab_size			= strtab ? strtab->size : 0;

	KERNEL_TABLE_OBJ->start							= (uint32_t)__kernel_start;
	KERNEL_TABLE_OBJ->end							= (uint32_t)__kernel_end;
}

uint8_t is_kernel_table_loaded() {
	return ELF32_TABLE_SIZE(KERNEL_TABLE_OBJ, sec) > 0 && ELF32_TABLE(KERNEL_TABLE_OBJ, hstr) != NULL;
}

typedef struct stackFrame {
	struct stackFrame* ebp;
	uint32_t eip;
} stackFrame_t;

void print_stack_frame(core_locals_t* locals, uint32_t address, bool ifFaulting) {
	if (!address)
		return;

	ELF32Obj_t* hdr = KERNEL_TABLE_OBJ;
	ELF32Symbol_t* symbol = NULL;

	while (true) {
		symbol = ELFFindNearestSymbolByAddress(hdr, STT_FUNC, address);

		if (!symbol && locals->current_thread && hdr == KERNEL_TABLE_OBJ) {
			hdr = locals->current_thread->parent->elf_obj;
			continue;
		}

		break;
	}

	ser_printf("%-3s ", ifFaulting ? "-->" : "");

    if (symbol) {
		uint8_t* name = hdr->sections.strtab_head + symbol->name;

		ser_printf("0x%08x -> %-25s at address 0x%08x ", address, name, symbol->value);
	} else {
		ser_printf("0x%08x -> %-25s at address 0x%08x ", address, "UNKNOWN", address);
	}

	if (locals->current_thread)
		ser_printf("in %s:%u ", locals->current_thread->parent->name, locals->current_thread->tid);

	ser_printf("\n");
}

void stack_trace(uint32_t maxFrames) {
    ser_printf("--[[						Stack trace begin						]]--\n");

	core_locals_t* locals = get_core_locals();

	stackFrame_t* stk;
	__asm__ volatile("movl %%ebp, %0" : "=r"(stk));

	if (locals->in_irq) {
		if (stk)
			for(; maxFrames > 0 && (uint32_t)stk != locals->irq_regs->ebp; maxFrames--) {
				if (!stk) break;
				print_stack_frame(locals, stk->eip, false);
				stk = stk->ebp;
			}

		stk = (stackFrame_t*)locals->irq_regs->ebp;

		print_stack_frame(locals, locals->irq_regs->eip, true);
		maxFrames--;
	}

	if (stk)
		for(; maxFrames > 0; maxFrames--) {
			if (!stk) break;
			print_stack_frame(locals, stk->eip, false);
			stk = stk->ebp;
		}

    ser_printf("--[[						Stack trace end			 				]]--\n");
}
