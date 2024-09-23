#include <int/isr.h>
#include <int/idt.h>
#include <sys/pic.h>
#include <sys/apic.h>
#include <sys/smp.h>

#include <drivers/serial.h>

int_handler_t handlers[IDT_ENTRIES] = {0};

void register_int_handler(uint8_t n, int_handler_t handler) {
	ser_printf("Register 0x%08x as handler for 0x%02x\n", handler, n);
    handlers[n] = handler;
}

char* get_interrupt_name(uint32_t i) {
	switch (i) {
		case 0: return "Division by zero exception";
		case 1: return "Debug exception";
		case 2: return "Non maskable interrupt";
		case 3: return "Breakpoint exception";
		case 4: return "Into detected overflow";
		case 5: return "Out of bounds exception";
		case 6: return "Invalid opcode exception";
		case 7: return "No coprocessor exception";
		case 8: return "Double fault";
		case 9: return "Coprocessor segment overrun";
		case 10: return "Bad TSS";
		case 11: return "Segment not present";
		case 12: return "Stack fault";
		case 13: return "General protection fault";
		case 14: return "Page fault";
		case 15: return "Unknown interrupt exception";
		case 16: return "Coprocessor fault";
		case 17: return "Alignment check exception";
		case 18: return "Machine check exception";

		case 32: return "System timer interrupt";
		case 33: return "Keyboard interrupt";

		case 128: return "System call";

		default: return "Unknown exception";
	}
}

// IVT Offset | INT #     | Description
// -----------+-----------+-----------------------------------
// 0x0000     | 0x00      | Divide by 0
// 0x0004     | 0x01      | Reserved
// 0x0008     | 0x02      | NMI Interrupt
// 0x000C     | 0x03      | Breakpoint (INT3)
// 0x0010     | 0x04      | Overflow (INTO)
// 0x0014     | 0x05      | Bounds range exceeded (BOUND)
// 0x0018     | 0x06      | Invalid opcode (UD2)
// 0x001C     | 0x07      | Device not available (WAIT/FWAIT)
// 0x0020     | 0x08      | Double fault
// 0x0024     | 0x09      | Coprocessor segment overrun
// 0x0028     | 0x0A      | Invalid TSS
// 0x002C     | 0x0B      | Segment not present
// 0x0030     | 0x0C      | Stack-segment fault
// 0x0034     | 0x0D      | General protection fault
// 0x0038     | 0x0E      | Page fault
// 0x003C     | 0x0F      | Reserved
// 0x0040     | 0x10      | x87 FPU error
// 0x0044     | 0x11      | Alignment check
// 0x0048     | 0x12      | Machine check
// 0x004C     | 0x13      | SIMD Floating-Point Exception
// 0x00xx     | 0x14-0x1F | Reserved
// 0x0xxx     | 0x20-0xFF | User definable

extern void _sched_runner(core_locals_t* locals);

void isr_handler(core_regs_t* _regs) {
	core_regs_base_t* regs = (core_regs_base_t*)_regs;

	core_locals_t* locals = get_core_locals();
	if (!locals)
		panic("isr_handler called with no core_locals!");

	if (locals->in_irq) {
		locals->nested_irq = true;
		// TODO: Proper handling of nested IRQs
		ser_printf("Nested IRQ %d (prev IRQ %d) on core %u\n", regs->int_num, locals->irq_regs->base.int_num, locals->core_id);
	} else {
		locals->in_irq = true;
		locals->irq_regs = _regs;
	}

	// if (regs->int_num != 32 && regs->int_num != 240) {
	// 	ser_printf("Interrupt %d [%s] on core %u (in_irq %u) ", regs->int_num, get_interrupt_name(regs->int_num), locals->core_id, locals->in_irq);
	// 	if (locals->current_thread)
	// 		ser_printf("thread: %s (tid %u)\n", locals->current_thread->parent->name, locals->current_thread->tid);
	// 	else
	// 		ser_printf("idle\n");
	// }

	if ((regs->int_num == 0x0D && !regs->v86_mode) || regs->int_num == 0x06) {
		ser_printf("%s on core %u: %08x\n", get_interrupt_name(regs->int_num), locals->core_id, regs->int_err);
		ser_printf("Type: %s\n", regs->err.external ? "External" : "Internal");
		ser_printf("Table: ");
		switch (regs->err.table_indicator) {
			case 0b00: ser_printf("GDT\n"); break;
			case 0b01: ser_printf("LDT\n"); break;
			case 0b10: ser_printf("IDT\n"); break;
			case 0b11: ser_printf("Unknown\n"); break;
		}
		ser_printf("Selector: 0x%08x\n", regs->err.selector);

		ser_printf("Code: ");
		for (uint32_t p = regs->eip - 8; p < regs->eip + 8; p++)
			ser_printf("%02x ", *(uint8_t*)p);
		ser_printf("\n");

		panic(get_interrupt_name(regs->int_num));
	}

	int_handler_t handler = handlers[regs->int_num];
	if (handler)
		handler(locals);
	else if (regs->int_num < 32) {
		ser_printf("Unhandled interrupt 0x%02x [%s]\n", regs->int_num, get_interrupt_name(regs->int_num));
		panic(get_interrupt_name(regs->int_num));
	}

	if (locals->need_resched)
		_sched_runner(locals);

	if (locals->nested_irq) {
		locals->nested_irq = false;
	} else {
		_regs = locals->irq_regs;
		locals->in_irq = false;
	}

	pic_sendEOI(regs->int_num);
	apic_EOI();
}