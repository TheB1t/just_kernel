#pragma once

#include <klibc/stdlib.h>
#include <mm/mm.h>

void        pmm_memory_setup(uint8_t* new_bitmap, uint32_t new_bitmap_bytes, multiboot_memory_map_t* mmap, uint32_t mmap_len);
void*       pmm_alloc(uint32_t size);
void*       pmm_alloc_from(void* addr, uint32_t size);
void        pmm_unalloc(void* addr, uint32_t size);

uint32_t    pmm_get_used_mem();
uint32_t    pmm_get_free_mem();
uint32_t    pmm_get_total_mem();