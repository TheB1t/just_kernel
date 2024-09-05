#include <int/isr.h>
#include <int/idt.h>
#include <sys/pic.h>
#include <sys/apic.h>
#include <sys/smp.h>

#include <drivers/serial.h>

int_handler_t handlers[IDT_ENTRIES] = {0};

void register_int_handler(uint8_t n, int_handler_t handler) {
	sprintf("Register 0x%08x as handler for %d\n", handler, n);
    handlers[n] = handler;
}

void isr_handler() {
	core_locals_t* locals = get_core_locals();
	core_regs_t* regs = &locals->irq_regs;

	// if (regs->int_num != 32 && regs->int_num != 240)
	// 	isprintf("Interrupt %d on core %u (in_irq %u)\n", regs->int_num, locals->core_index, locals->in_irq);

	int_handler_t handler = handlers[regs->int_num];
	if (handler)
		handler(locals);

	if (regs->int_num == 0x0D) {
		isprintf("General Protection Fault on core %u: %08x\n", locals->core_index, regs->int_err);
		panic("General Protection Fault");
	}

	pic_sendEOI(regs->int_num);
	apic_EOI();
}