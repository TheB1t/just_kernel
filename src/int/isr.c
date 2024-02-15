#include <int/isr.h>
#include <int/idt.h>
#include <sys/pic.h>
#include <sys/apic.h>

#include <drivers/serial.h>

int_handler_t handlers[IDT_ENTRIES];

void register_int_handler(uint8_t n, int_handler_t handler) {
	sprintf("Register 0x%08x as handler for %d\n", handler, n);
    handlers[n] = handler;
}

void isr_handler(int_reg_t* regs) {
	int_handler_t handler = handlers[regs->int_num];
	if (handler)
		handler(regs);

	pic_sendEOI(regs->int_num);
	apic_EOI();
}