#include <mm/pmm.h>
#include <mm/vmm.h>
#include <klibc/math.h>

#include <int/isr.h>

#include <drivers/serial.h>

uint8_t* bitmap;
uint32_t bitmap_max_page;

uint32_t total_memory = 0;
uint32_t used_memory = 0;
uint32_t available_memory = 0;

void pmm_set_bit(uint32_t page) {
    uint8_t bit = page % 8;
    uint32_t byte = page / 8;

    bitmap[byte] |= (1 << bit);
}

void pmm_clear_bit(uint32_t page) {
    uint8_t bit = page % 8;
    uint32_t byte = page / 8;

    bitmap[byte] &= ~(1 << bit);
}

uint8_t pmm_get_bit(uint32_t page) {
    uint8_t bit = page % 8;
    uint32_t byte = page / 8;

    return bitmap[byte] & (1 << bit);
}

void pmm_memory_setup(uint8_t* new_bitmap, uint32_t bitmap_bytes, multiboot_memory_map_t* mmap, uint32_t mmap_len) {
    bitmap = new_bitmap;
    bitmap_max_page = bitmap_bytes * 8;

    memset(bitmap, 0xFF, bitmap_bytes);

    for (uint32_t i = 0; i < mmap_len; i++) {
        uint32_t rounded_block_start = ROUND_UP(mmap[i].addr_low, PAGE_SIZE);
        uint32_t rounded_block_end = ROUND_UP((mmap[i].addr_low + mmap[i].len_low), PAGE_SIZE);
        uint32_t rounded_block_len = rounded_block_end - rounded_block_start;
        
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE && mmap[i].addr_low >= 0x100000) {
            for (uint32_t i = 0; i < rounded_block_len / PAGE_SIZE; i++)
                pmm_clear_bit(i + (rounded_block_start / PAGE_SIZE));

            available_memory += rounded_block_len;
        }
    }

    total_memory = available_memory;

    sprintf("[PMM] Total memory %d KB (max page %d)\n", total_memory / 1024, bitmap_max_page);
}

static uint32_t find_free_page(uint32_t pages) {
    uint32_t found_pages = 0;
    uint32_t first_page = 0;

    for (uint32_t i = 0; i < bitmap_max_page; i++) {
        if (!pmm_get_bit(i)) {
            found_pages += 1;
            if (first_page == 0) {
                first_page = i;
            }
        } else {
            found_pages = 0;
            first_page = 0;
        }

        if (found_pages == pages)
            return first_page;
    }

    sprintf("[PMM] Error! Couldn't find free memory!\n");
    while(1) asm volatile("hlt");

    return 0;
}

void* pmm_alloc(uint32_t size) {
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t free_page = find_free_page(pages);

    for (uint32_t i = 0; i < pages; i++)
        pmm_set_bit(free_page + i);

    available_memory -= pages * PAGE_SIZE;
    used_memory += pages * PAGE_SIZE;

    return (void*)(free_page * PAGE_SIZE);
}

void* pmm_alloc_from(void* addr, uint32_t size) {
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t start_page = (uint32_t)addr / PAGE_SIZE;

    for (uint32_t i = 0; i < pages; i++)
        if (pmm_get_bit(start_page + i))
            return (void*)0;

    for (uint32_t i = 0; i < pages; i++)
        pmm_set_bit(start_page + i);

    available_memory -= pages * PAGE_SIZE;
    used_memory += pages * PAGE_SIZE;

    return (void*)(start_page * PAGE_SIZE);
}

void pmm_unalloc(void* addr, uint32_t size) {
    if ((uint32_t)addr % PAGE_SIZE != 0)
        sprintf("[PMM] freeing bad address, caller: 0x%08x, addr: 0x%08x\n", __builtin_return_address(0), addr);

    uint32_t page = ((uint32_t) addr & ~(0xfff)) / PAGE_SIZE;
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    if (page + pages > bitmap_max_page) {
        sprintf("[PMM] trying to free bad memory: 0x%08x. Size: 0x%08x\n", addr, size);
        while(1) asm volatile("hlt");
    }

    for (uint32_t i = 0; i < pages; i++)
        pmm_clear_bit(page + i);

    available_memory += pages * PAGE_SIZE;
    used_memory -= pages * PAGE_SIZE;
}

uint32_t pmm_get_free_mem() {
    return available_memory;
}

uint32_t pmm_get_used_mem() {
    return used_memory;
}

uint32_t pmm_get_total_mem() {
    return total_memory;
}