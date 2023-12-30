#include <drivers/serial.h>

uint16_t _active_port = 0;

void sprintf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    _vprintf(serial_writeChar, format, va);
    va_end(va);
}

int32_t serial_init(uint16_t port, uint16_t baud) {
	port_outb(port + 1, 0x00);									// Disable all interrupts
	port_outb(port + UART_LCR,		UART_LCR_DLAB);				// Enable DLAB (set baud rate divisor)
	port_outb(port + UART_DL_LOW,	(baud & 0x00FF) >> 0);		// Set divisor to 3 (lo byte) 38400 baud
	port_outb(port + UART_DL_HIGH,	(baud & 0xFF00) >> 8);		// (hi byte)
	port_outb(port + UART_LCR,		UART_LCR_8BIT);				// 8 bits, no parity, one stop bit
	port_outb(port + UART_FCR,		0xC7);						// Enable FIFO, clear them, with 14-byte threshold
	port_outb(port + UART_MCR,		0x0B);						// IRQs enabled, RTS/DSR set
	port_outb(port + UART_MCR,		0x1E);						// Set in loopback mode, test the serial chip
	port_outb(port + UART_DAR,		0xAE);						// Test serial chip (send byte 0xAE and check if serial returns same byte)
	
	// Check if serial is faulty (i.e: not same byte as sent)
	if (port_inb(port + UART_DAR) != 0xAE)
		return 1;
 
	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	port_outb(port + UART_MCR, 0x0F);

    if (!_active_port)
        _active_port = port;

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