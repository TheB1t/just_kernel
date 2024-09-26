#pragma once

#include <klibc/stdlib.h>
#include <klibc/stdarg.h>
#include <io/ports.h>

#define UART_DAR		0
#define UART_IER		1
#define UART_DL_LOW		0
#define UART_DL_HIGH	1
#define UART_FCR		2
#define UART_LCR		3
#define UART_MCR		4
#define UART_LSR		5
#define UART_MSR		6

#define UART_LCR_DLAB	0b10000000
#define UART_LCR_5BIT	0b00000000
#define UART_LCR_6BIT	0b00000001
#define UART_LCR_7BIT	0b00000010
#define UART_LCR_8BIT	0b00000011

#define UART_BAUD_600		0x00C0
#define UART_BAUD_1200		0x0060
#define UART_BAUD_1800		0x0040
#define UART_BAUD_2000		0x003A
#define UART_BAUD_2400		0x0030
#define UART_BAUD_3600		0x0020
#define UART_BAUD_4800		0x0018
#define UART_BAUD_7200		0x0010
#define UART_BAUD_9600		0x000C
#define UART_BAUD_19200		0x0006
#define UART_BAUD_38400		0x0003
#define UART_BAUD_57600		0x0002
#define UART_BAUD_115200	0x0001

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define COM5 0x5F8
#define COM6 0x4F8
#define COM7 0x5E8
#define COM8 0x4E8

void		ser_printf(char* format, ...);

int32_t		serial_init(uint16_t port, uint16_t baud);
char		serial_getc();
uint32_t	serial_gets(char* str);
void		serial_putc(char a);
uint32_t	serial_puts(char* str);