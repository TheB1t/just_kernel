#pragma once

#include <klibc/stdlib.h>
#include <mm/mm.h>

#define VMM_4K_PERM_MASK (~(0xfff))

#define VMM_PRESENT         BIT(0)
#define VMM_WRITE           BIT(1)
#define VMM_USER            BIT(2)
#define VMM_WRITE_THROUGH   BIT(3)
#define VMM_NO_CACHE        BIT(4)
#define VMM_ACCESS          BIT(5)
#define VMM_DIRTY           BIT(6)
#define VMM_HUGE            BIT(7)

typedef struct {
    uint32_t tid;
    uint32_t pid;
    uint32_t off;
} vmm_table_off_t;

typedef union {
    uint32_t raw;
    struct {
        uint32_t present : 1;
        uint32_t write : 1;
        uint32_t user : 1;
        uint32_t write_through : 1;
        uint32_t no_cache : 1;
        uint32_t access : 1;
        uint32_t dirty : 1;
        uint32_t pat : 1;
        uint32_t global : 1;
        uint32_t available : 3;
        uint32_t address : 20;
    };
} vmm_entry_t;

typedef struct {
    vmm_entry_t entries[1024];
} vmm_table_t;


vmm_table_t* vmm_get_cr3();
vmm_table_t* vmm_replace_cr3(vmm_table_t* new);
void vmm_set_cr3(vmm_table_t* new);
void vmm_invlpg(void* new);

vmm_table_off_t vmm_virt_to_offs(void* virt);
void* vmm_offs_to_virt(vmm_table_off_t offs);

void* virt_to_phys(void* virt, vmm_table_t* table0);

uint8_t is_mapped(void* data, vmm_table_t* cr3);
uint8_t range_mapped(void* data, uint32_t size);

int32_t vmm_map_pages(void* phys, void* virt, vmm_table_t* page_dir, uint32_t count, uint16_t perms);
int32_t vmm_remap_pages(void* phys, void* virt, vmm_table_t* page_dir, uint32_t count, uint16_t perms);
int32_t vmm_unmap_pages(void* virt, vmm_table_t* page_dir, uint32_t count);

int32_t vmm_map(void* phys, void* virt, uint32_t count, uint16_t perms);
int32_t vmm_remap(void* phys, void* virt, uint32_t count, uint16_t perms);
int32_t vmm_unmap(void* virt, uint32_t count);

void* vmm_virt_to_phys(void* virt);
void vmm_memory_setup(multiboot_memory_map_t* mmap, uint32_t mmap_len);
void* vmm_fork_kernel_space();
void vmm_free_cr3(vmm_table_t* table0);