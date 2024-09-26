#pragma once

#include <klibc/stdlib.h>

#define  BMP_MAGIC 0x4D42

typedef struct {
	uint16_t type;
	uint32_t size;
	uint16_t reserved[2];
	uint32_t off_bits;
} __attribute__((packed)) bmp_header_t;

typedef struct {
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bit_count;
	uint32_t compression;
	uint32_t size_image;
	uint32_t unused[25];
} __attribute__((packed)) bmp_info_header_t;

void bmp_draw(uint32_t x, uint32_t y, int32_t fd);