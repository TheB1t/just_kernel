#pragma once

#include <klibc/stdlib.h>
#include <mm/mm.h>

void        pmm_memory_setup(multiboot_mmap_t* mmap, uint32_t mmap_len);
void*       pmm_alloc(uint32_t size);
void*       pmm_alloc_from(void* addr, uint32_t size);
void        pmm_unalloc(void* addr, uint32_t size);

uint32_t    pmm_used_mem();
uint32_t    pmm_free_mem();
uint32_t    pmm_total_mem();