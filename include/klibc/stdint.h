#pragma once

#define NULL (void*)0

typedef	unsigned int	uint32_t;
typedef			 int	int32_t;
typedef	unsigned short	uint16_t;
typedef			 short	int16_t;
typedef	unsigned char	uint8_t;
typedef 		 char	int8_t;
typedef uint32_t size_t;

typedef struct {
	uint32_t low;
	uint32_t high;
} uint64_t;