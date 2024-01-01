#include <klibc/stdlib.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

void* kmalloc(uint32_t size) {
    if (!size) return NULL;

    uint32_t size_data = (uint32_t)pmm_alloc(size + 0x1000);
    uint32_t page_count = ((size + 0x1000) + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t last_page = size_data + ((page_count - 1) * PAGE_SIZE);

    vmm_map((void*)size_data, (void*)size_data, page_count, VMM_PRESENT | VMM_WRITE);

    *(uint32_t*)size_data = size + 0x1000;
    *(uint32_t*)(size_data + 8) = MALLOC_SIGNATURE;

    vmm_unmap((void*)size_data, 1);

    if (size_data % PAGE_SIZE != 0)
        panic("malloc bug");

    return (void*)(size_data + 0x1000);
}

void kfree(void* addr) {
    if (!addr) return;
    if (!is_mapped(addr)) {
        sprintf("[kfree] trying to free unmapped memory! addr: 0x%08x, caller: 0x%08x\n", addr, __builtin_return_address(0));
        return;
    }

    assert((uint32_t)addr % PAGE_SIZE == 0);

    vmm_map((void*)((uint32_t)addr - 0x1000), (void*)((uint32_t)addr - 0x1000), 1, VMM_PRESENT | VMM_WRITE);
    uint32_t*   size_data = (uint32_t*)((uint32_t)addr - 0x1000);
    uint32_t*   signature = (uint32_t*)((uint32_t)addr - 0x0ff8);
    uint32_t    page_count = ((*size_data - 0x1000) + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t    last_page = (uint32_t) addr + ((page_count - 1) * PAGE_SIZE);

    if (*signature != MALLOC_SIGNATURE) {
        sprintf("[kfree] signature check failed in kfree()! addr: 0x%08x, caller: 0x%08x\n", addr, __builtin_return_address(0));
        panic("malloc bug");
    }

    void *phys = virt_to_phys((void *) size_data, (vmm_table_t*)vmm_get_base());
    if ((uint32_t) phys != 0xFFFFFFFF) {
        pmm_unalloc(phys, *size_data);
    }

    vmm_unmap((void*)size_data, page_count);
}


void panic(char* msg) {
    kprintf("Kernel PANIC!!! [%s]\n", msg);

    while (1)
        asm volatile("hlt");
}