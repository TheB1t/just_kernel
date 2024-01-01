#pragma once

#include <klibc/stdint.h>
#include <klibc/string.h>
#include <klibc/stdio.h>

#include <drivers/serial.h>

#define stringify_param(x) #x
#define stringify(x) stringify_param(x)
#define assert(statement) do { if (!(statement)) { sprintf("Assert Failed. Return address: 0x%08x\n", __builtin_return_address(0)); panic("Assert failed. File: " __FILE__ ", Line: " stringify(__LINE__) " Condition: assert(" #statement ");"); } } while (0)
#define MALLOC_SIGNATURE 0xf100f333

void*   kmalloc(uint32_t size);
void    kfree(void *addr);
void*   kcalloc(uint32_t size);
void*   krealloc(void *addr, uint32_t new_size);
void    panic(char* msg);