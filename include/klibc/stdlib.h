#pragma once

#include <klibc/stdint.h>
#include <klibc/string.h>
#include <klibc/stdio.h>
#include <klibc/bool.h>
#include <klibc/errno.h>

#include <drivers/serial.h>

#define stringify_param(x) #x
#define stringify(x) stringify_param(x)
#define assert(statement) do { if (!(statement)) { ser_printf("Assert Failed. File: " __FILE__ ", Line: " stringify(__LINE__) " Condition: assert(" #statement "), Return address: 0x%08x\n", __builtin_return_address(0)); panic("Assert failed"); } } while (0)

#define UNREACHEBLE     for(;;); __builtin_unreachable()
#define UNUSED          __attribute__((unused))

void*   kmalloc(uint32_t size);
void*   kmalloc_a(uint32_t size);
void    kfree(void *addr);
void*   kcalloc(uint32_t size);
void    panic(char* msg);