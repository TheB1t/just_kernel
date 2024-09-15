#include <drivers/serial.h>
#include <klibc/lock.h>
#include <sys/smp.h>

lock_t sprintf_lock		= INIT_LOCK(sprintf_lock);
lock_t irq_sprintf_lock	= INIT_LOCK(irq_sprintf_lock);

uint16_t _active_port = 0;

void ser_printf(const char* format, ...) {
	if (!_active_port)
		return;

	core_locals_t* locals = get_core_locals();
	if (!locals)
		goto generic;

	if (!is_mapped((void*)locals, vmm_get_cr3()))
		goto generic;

	if (locals->in_irq) {
		lock(irq_sprintf_lock);
		va_list va;
		va_start(va, format);
		_vprintf(serial_writeChar, format, va);
		va_end(va);
		unlock(irq_sprintf_lock);
		return;
	}

generic:
	lock(sprintf_lock);
	va_list va;
	va_start(va, format);
	_vprintf(serial_writeChar, format, va);
	va_end(va);
	unlock(sprintf_lock);
}

int32_t serial_init(uint16_t port, uint16_t baud) {
	port_outb(port + UART_IER,		0x00);						// Disable all interrupts
	port_outb(port + UART_LCR,		UART_LCR_DLAB);				// Enable DLAB (set baud rate divisor)
	port_outb(port + UART_DL_LOW,	(baud & 0x00FF) >> 0);		// Set divisor to 3 (lo byte) 38400 baud
	port_outb(port + UART_DL_HIGH,	(baud & 0xFF00) >> 8);		// (hi byte)
	port_outb(port + UART_LCR,		UART_LCR_8BIT);				// 8 bits, no parity, one stop bit
	port_outb(port + UART_FCR,		0xC7);						// Enable FIFO, clear them, with 14-byte threshold
	port_outb(port + UART_MCR,		0x0B);						// IRQs enabled, RTS/DSR set
	port_outb(port + UART_MCR,		0x1E);						// Set in loopback mode, test the serial chip
	port_outb(port + UART_MCR,		0x0F);

    if (!_active_port) {
        _active_port = port;
	}

	return 0;
}

int32_t serial_recived(uint16_t port) {
	return port_inb(port + UART_LSR) & 1;
}

int serial_isTransmitEmpty(uint16_t port) {
	return port_inb(port + UART_LSR) & 0x20;
}

char serial_readChar() {
   while (serial_recived(_active_port) == 0);
   return port_inb(_active_port);
}

uint32_t serial_readString(char* str) {
	uint32_t readed = 0;
	while (1) {
		char c = serial_readChar(_active_port);
		str[readed++] = c;
		if (c == '\0')
			break;
	}
	return readed;
}

void serial_writeChar(char c) {
	while (serial_isTransmitEmpty(_active_port) == 0);
	port_outb(_active_port, c);

	if (c == '\n')
		serial_writeChar('\r');
}

uint32_t serial_writeString(char* str) {
	uint32_t writed = 0;
	while (1) {
		char c = str[writed++];
		serial_writeChar(c);
		if (c == '\0')
			break;
	}
	return writed;
}