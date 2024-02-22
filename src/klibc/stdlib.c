#include <klibc/stdlib.h>
#include <mm/heap.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

extern Heap_t* kernel_heap;

void* kmalloc(uint32_t size) {
    return (void*)heap_malloc(kernel_heap, size, 0);
}

void kfree(void* addr) {
    heap_free(kernel_heap, addr);
}

void *kcalloc(uint32_t size) {
    void *buffer = kmalloc(size);

    memset((uint8_t*)buffer, 0, size);
    return buffer;
}

void panic(char* msg) {
    kprintf("Kernel PANIC!!! [%s]\n", msg);

    while (1)
        asm volatile("hlt");
}