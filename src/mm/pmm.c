#include <mm/pmm.h>
#include <klibc/math.h>

#include <int/isr.h>

#define BITMAP_MAX_PAGE ((((uint32_t)-1) / PAGE_SIZE) / 8)

extern symbol stack;

uint8_t bitmap[BITMAP_MAX_PAGE] = {0};

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

void pmm_memory_setup(multiboot_memory_map_t* mmap, uint32_t mmap_len) {
    multiboot_memory_map_t* start = &mmap[0];
    multiboot_memory_map_t* end = &mmap[mmap_len - 1];

    uint32_t mem_length = ROUND_UP((uint32_t)((end->addr + end->len) - start->addr), PAGE_SIZE);
    mem_length = mem_length ? mem_length : 0xFFFFFFFF;

    memset(bitmap, 0xFF, BITMAP_MAX_PAGE);

    for (uint32_t i = 0; i < mmap_len; i++) {
        uint32_t rounded_block_start = ROUND_UP((uint32_t)mmap[i].addr, PAGE_SIZE);
        uint32_t rounded_block_end = ROUND_UP((uint32_t)(mmap[i].addr + mmap[i].len), PAGE_SIZE);
        uint32_t rounded_block_len = rounded_block_end - rounded_block_start;

        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE && (uint32_t)mmap[i].addr >= 0x100000) {
            for (uint32_t i = 0; i < rounded_block_len / PAGE_SIZE; i++)
                pmm_clear_bit(i + (rounded_block_start / PAGE_SIZE));

            available_memory += rounded_block_len;
        }
    }

    total_memory = available_memory;

    ser_printf("[PMM] kernel: 0x%08x - 0x%08x\n", __kernel_start, __kernel_end);
}

static uint32_t find_free_page(uint32_t pages) {
    uint32_t found_pages = 0;
    uint32_t first_page = 0;

    for (uint32_t i = 0; i < BITMAP_MAX_PAGE; i++) {
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

    ser_printf("[PMM] Error! Couldn't find free memory!\n");
    while(1)
        asm volatile("hlt");

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
        ser_printf("[PMM] freeing bad address, caller: 0x%08x, addr: 0x%08x\n", __builtin_return_address(0), addr);

    uint32_t page = ((uint32_t) addr & ~(0xfff)) / PAGE_SIZE;
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    if (page + pages > BITMAP_MAX_PAGE) {
        ser_printf("[PMM] trying to free bad memory: 0x%08x. Size: 0x%08x\n", addr, size);
        while(1)
            asm volatile("hlt");
    }

    for (uint32_t i = 0; i < pages; i++)
        pmm_clear_bit(page + i);

    available_memory += pages * PAGE_SIZE;
    used_memory -= pages * PAGE_SIZE;
}

uint32_t pmm_free_mem() {
    return available_memory;
}

uint32_t pmm_used_mem() {
    return used_memory;
}

uint32_t pmm_total_mem() {
    return total_memory;
}

#define _B(V) ((V) / 1024)
#define KB(V) _B(V)
#define MB(V) _B(KB(V))

void pmm_print_memory_stats() {
    kprintf("[PMM] Total memory: %u MB (%u KB)\n", MB(total_memory), KB(total_memory));
    kprintf("[PMM] Available memory: %u MB (%u KB)\n", MB(available_memory), KB(available_memory));
    kprintf("[PMM] Used memory: %u MB (%u KB)\n", MB(used_memory), KB(used_memory));
}