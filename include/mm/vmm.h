#pragma once

#include <klibc/stdlib.h>
#include <mm/mm.h>
#include <int/isr.h>

#define VMM_4K_PERM_MASK (~(0xfff))

#define VMM_PRESENT         (1 << 0)
#define VMM_WRITE           (1 << 1)
#define VMM_USER            (1 << 2)
#define VMM_WRITE_THROUGH   (1 << 3)
#define VMM_NO_CACHE        (1 << 4)
#define VMM_ACCESS          (1 << 5)
#define VMM_DIRTY           (1 << 6)
#define VMM_HUGE            (1 << 7)

typedef struct {
    uint32_t p0_off;
    uint32_t p1_off;
} vmm_table_off_t;

typedef struct {
    uint32_t entries[1024];
} vmm_table_t;

int         vmm_map(void* phys, void* virt, uint32_t count, uint16_t perms);
int         vmm_remap(void* phys, void* virt, uint32_t count, uint16_t perms);
int         vmm_unmap(void* virt, uint32_t count);
int         vmm_map_pages(void* phys, void* virt, void* ptr, uint32_t count, uint16_t perms);
int         vmm_remap_pages(void* phys, void* virt, void* ptr, uint32_t count, uint16_t perms);
int         vmm_unmap_pages(void* virt, void* ptr, uint32_t count);
void        vmm_set_base(uint32_t new);
void*       vmm_virt_to_phys(void* virt);
uint8_t     is_mapped(void* data);
uint8_t     range_mapped(void* data, uint32_t size);
uint32_t    vmm_get_base();

uint32_t    get_entry(vmm_table_t* cur_table, uint32_t offset);

void        vmm_memory_setup(uint32_t kernel_start, uint32_t kernel_len);