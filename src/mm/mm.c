#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

extern symbol stack;

uint32_t mm_memory_setup(multiboot_t* bootloader_info) {
    if (!(bootloader_info->flags & MULTIBOOT_FLAG_MMAP))
        panic("Bootloader can't store memory map!\n");

    uint32_t kernel_start = (uint32_t)__kernel_start;
    uint32_t kernel_end = (uint32_t)__kernel_end;

    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)bootloader_info->mmap_addr;
    uint32_t mmap_length = bootloader_info->mmap_length / sizeof(multiboot_memory_map_t);

    multiboot_memory_map_t* start = &mmap[0];
    multiboot_memory_map_t* end = &mmap[mmap_length - 1];

    uint32_t mem_length = ROUND_UP((end->addr_low + end->len_low) - start->addr_low, PAGE_SIZE);
    mem_length = mem_length ? mem_length : 0xFFFFFFFF;

    sprintf("[MM] Memory map provided by bootloader:\n");
    #define GET_TYPE(v, t) v[i].type == MULTIBOOT_MEMORY_##t ? #t :
    for (uint32_t i = 0; i < mmap_length; i++)
        sprintf("   [%d] 0x%08x - 0x%08x [%s]\n", i, mmap[i].addr_low, mmap[i].addr_low + mmap[i].len_low, 
            GET_TYPE(mmap, AVAILABLE)
            GET_TYPE(mmap, RESERVED)
            GET_TYPE(mmap, ACPI_RECLAIMABLE)
            GET_TYPE(mmap, NVS)
            GET_TYPE(mmap, BADRAM)
            "UNKNOWN"
        );

    uint8_t* bitmap = (uint8_t*)((kernel_end + PAGE_SIZE - 1) & VMM_4K_PERM_MASK);

    if ((uint32_t)bootloader_info + sizeof(multiboot_t) > (uint32_t)bitmap)
        bitmap = (uint8_t*)((uint32_t)bootloader_info + sizeof(multiboot_t));

    uint32_t bitmap_bytes = ((mem_length / PAGE_SIZE) + 8 - 1) / 8;
    uint32_t bitmap_max_page = bitmap_bytes * 8;

    pmm_memory_setup(bitmap, bitmap_bytes, mmap, mmap_length);

    sprintf("[MM] kernel: 0x%08x - 0x%08x\n", kernel_start, kernel_end);
    sprintf("[MM] bitmap: 0x%08x - 0x%08x\n", bitmap, bitmap + bitmap_bytes);
    sprintf("[MM] stack: 0x%08x - 0x%08x\n", stack, stack + 4096);

    uint32_t kernel_len = ((uint32_t)bitmap + bitmap_bytes) - kernel_start;

    if (!pmm_alloc_from((void*)kernel_start, kernel_len + PAGE_SIZE)) {
        sprintf("[MM] Failed to allocate kernel address space!");
        panic("Can't init memory manager");
    }

    vmm_memory_setup(kernel_start, kernel_len + PAGE_SIZE);

    return 0;
}