#include <klibc/stdlib.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

void* kmalloc(uint32_t size) {
    if (!size) return NULL;

    void* addr = pmm_alloc(size + 0x1000);

    uint32_t*   header = (uint32_t*)addr;
    uint32_t    page_count = ((size + 0x1000) + PAGE_SIZE - 1) / PAGE_SIZE;

    vmm_map(addr, addr, page_count, VMM_PRESENT | VMM_WRITE);

    header[0] = size + 0x1000;
    header[1] = MALLOC_SIGNATURE;

    vmm_unmap(addr, 1);

    if ((uint32_t)addr % PAGE_SIZE != 0)
        panic("malloc bug");

    return addr + 0x1000;
}

void kfree(void* addr) {
    if (!addr) return;
    if (!is_mapped(addr)) {
        sprintf("[kfree] trying to free unmapped memory! addr: 0x%08x, caller: 0x%08x\n", addr, __builtin_return_address(0));
        return;
    }

    assert((uint32_t)addr % PAGE_SIZE == 0);

    addr -= 0x1000;

    vmm_map(addr, addr, 1, VMM_PRESENT | VMM_WRITE);
    uint32_t*   header = (uint32_t*)addr;
    uint32_t    page_count = (header[0] + PAGE_SIZE - 1) / PAGE_SIZE;

    if (header[1] != MALLOC_SIGNATURE) {
        sprintf("[kfree] signature check failed in kfree()! addr: 0x%08x, caller: 0x%08x\n", addr, __builtin_return_address(0));
        panic("malloc bug");
    }

    void* phys = virt_to_phys(addr, (vmm_table_t*)vmm_get_base());
    if ((uint32_t)phys != 0xFFFFFFFF)
        pmm_unalloc(phys, header[0]);

    vmm_unmap(addr, page_count);
}

void *kcalloc(uint32_t size) {
    void *buffer = kmalloc(size);

    memset((uint8_t *) buffer, 0, size);
    return buffer;
}

void *krealloc(void *addr, uint32_t new_size) {
    void *new_buffer = kcalloc(new_size);
    if (!addr)
        return new_buffer;

    addr -= 0x1000;

    vmm_map(addr, addr, 1, VMM_PRESENT | VMM_WRITE);
    uint32_t*   header = (uint32_t*)addr;
    void*       data = (uint32_t*)(addr + 0x1000);
    uint32_t    page_count = (header[0] + PAGE_SIZE - 1) / PAGE_SIZE;

    memcpy((uint8_t*)data, (uint8_t*)new_buffer, header[0] - 0x1000);

    vmm_unmap(addr, 1);
    kfree(addr + 0x1000);
    return new_buffer;
}

void panic(char* msg) {
    kprintf("Kernel PANIC!!! [%s]\n", msg);

    while (1)
        asm volatile("hlt");
}