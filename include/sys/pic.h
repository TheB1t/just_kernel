#pragma once

#include <klibc/stdlib.h>

#define PIC1			0x20
#define PIC2			0xA0
#define PIC1_COMMAND	PIC1
#define PIC1_DATA		(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA		(PIC2+1)

#define PIC_EOI			0x20

#define ICW1_ICW4		0x01
#define ICW1_SINGLE		0x02
#define ICW1_INTERVAL4	0x04
#define ICW1_LEV		0x08
#define ICW1_INIT		0x10

#define ICW4_8086		0x01
#define ICW4_AUTO		0x02
#define ICW4_BUF_SLAVE	0x08
#define ICW4_BUF_MASTER	0x0C
#define ICW4_SFNM		0x10

uint32_t pic_remap(uint8_t offset1, uint8_t offset2);
void pic_disable();

void pic_sendEOI_master();
void pic_sendEOI_slave();
void pic_sendEOI(uint32_t isr);