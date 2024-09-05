#pragma once

#include <klibc/stdlib.h>
#include <multiboot.h>

#define PAGE_SIZE 0x1000

#define ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

typedef void* symbol[];

extern symbol __kernel_end;
extern symbol __kernel_start;
extern symbol __kernel_code_start;
extern symbol __kernel_code_end;

void mm_memory_setup(multiboot_t* bootloader_info);