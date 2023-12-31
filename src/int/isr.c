#include <int/isr.h>
#include <int/idt.h>
#include <sys/pic.h>

#include <drivers/serial.h>

int_handler_t handlers[IDT_ENTRIES];

void register_int_handler(uint8_t n, int_handler_t handler) {
    handlers[n] = handler;
}

void isr_handler(int_reg_t* regs) {
	if (regs->int_num >= 32 && regs->int_num <= 47) {
		if (regs->int_num >= 40)
			pic_sendEOI_slave();

		pic_sendEOI_master();
	}

    sprintf("Got interrupt: %d\n", regs->int_num);

	int_handler_t handler = handlers[regs->int_num];
	if (handler)
		handler(regs);
}