#include <sys/pic.h>
#include <io/ports.h>

void pic_remap(int offset1, int offset2) {
	uint8_t a1, a2;

	a1 = port_inb(PIC1_DATA);
	a2 = port_inb(PIC2_DATA);

	port_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	port_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

	port_outb(PIC1_DATA, offset1);
	port_outb(PIC2_DATA, offset2);

	port_outb(PIC1_DATA, 0x04);
	port_outb(PIC2_DATA, 0x02);

	port_outb(PIC1_DATA, ICW4_8086);
	port_outb(PIC2_DATA, ICW4_8086);

	port_outb(PIC1_DATA, a1);
	port_outb(PIC2_DATA, a2);
}

void pic_disable() {
	port_outb(PIC1_DATA, 0xff);
	port_outb(PIC2_DATA, 0xff);
}

void pic_sendEOI_master() {
	port_outb(PIC1_COMMAND, PIC_EOI);
}

void pic_sendEOI_slave() {
	port_outb(PIC2_COMMAND, PIC_EOI);
}