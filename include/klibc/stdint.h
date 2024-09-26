#pragma once

#define BIT(n)                          (1 << (n))

#define SET_MASK(val, mask)             ((val) |=   (mask))
#define CLEAR_MASK(val, mask)           ((val) &=  ~(mask))
#define APPLY_MASK(val, mask)           ((val) &    (mask))
#define TEST_MASK(val, mask)            (APPLY_MASK(val, mask) == (mask))

#define SET_BIT(val, bit)               SET_MASK(val, BIT(bit))
#define CLEAR_BIT(val, bit)             CLEAR_MASK(val, BIT(bit))
#define TEST_BIT(val, bit)              TEST_MASK(val, BIT(bit))

#define NULL (void*)0

typedef	unsigned long long	uint64_t;
typedef			 long long	int64_t;
typedef	unsigned int		uint32_t;
typedef			 int		int32_t;
typedef	unsigned short		uint16_t;
typedef			 short		int16_t;
typedef	unsigned char		uint8_t;
typedef 		 char		int8_t;
typedef uint32_t size_t;