#include <mm/mm.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/heap.h>

Heap_t  _kernel_heap_base;
Heap_t* kernel_heap;

void mm_memory_setup(multiboot_info_t* mboot) {
    if (!(mboot->flags & MULTIBOOT_INFO_MEM_MAP))
        panic("Bootloader can't store memory map!\n");

    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mboot->mmap_addr;
    uint32_t mmap_len = mboot->mmap_length / sizeof(multiboot_memory_map_t);

    ser_printf("[MM] Memory map provided by bootloader:\n");
    #define GET_TYPE(v, t) v[i].type == MULTIBOOT_MEMORY_##t ? #t :
    for (uint32_t i = 0; i < mmap_len; i++)
        ser_printf("   [%d] 0x%08x - 0x%08x [%s]\n", i, (uint32_t)mmap[i].addr, (uint32_t)(mmap[i].addr + mmap[i].len),
            GET_TYPE(mmap, AVAILABLE)
            GET_TYPE(mmap, RESERVED)
            GET_TYPE(mmap, ACPI_RECLAIMABLE)
            GET_TYPE(mmap, NVS)
            GET_TYPE(mmap, BADRAM)
            "UNKNOWN"
        );

    pmm_memory_setup(mmap, mmap_len);

    uint32_t kernel_start = (uint32_t)__kernel_start;
    uint32_t kernel_end = (uint32_t)__kernel_end;

    if (!pmm_alloc_from((void*)kernel_start, kernel_end - kernel_start)) {
        ser_printf("[MM] Failed to allocate kernel address space!");
        panic("Can't init memory manager");
    }

    if (mboot->mods_count > 0) {
        multiboot_module_t* modules = (multiboot_module_t*)mboot->mods_addr;

        void* modules_block = (void*)modules[0].mod_start;
        uint32_t modules_block_size = modules[mboot->mods_count - 1].mod_end - modules[0].mod_start;

        if (!pmm_alloc_from((void*)modules_block, modules_block_size)) {
            ser_printf("[MM] Failed to allocate modules address space!");
            panic("Can't init memory manager");
        }
    }

    pmm_print_memory_stats();

    vmm_memory_setup(mmap, mmap_len);

    kernel_heap = createHeap((uint32_t)&_kernel_heap_base, (uint32_t)vmm_get_cr3(), 0x01000000, HEAP_MIN_SIZE, 0x02000000, VMM_WRITE);

    ser_printf("[MM] Kernel heap initialized\n");
}