#pragma once

#include <klibc/stdlib.h>
#include <sys/smp.h>
#include <mm/vmm.h>
#include <fs/elf/elf.h>

#define KERNEL_TABLE_OBJ	(&kernel_table_obj)

extern ELF32Obj_t		kernel_table_obj;

void					init_kernel_table(void* sybtabPtr, uint32_t size, uint32_t num, uint32_t shindex);
uint8_t					is_kernel_table_loaded();

void					stack_trace(uint32_t maxFrames);
