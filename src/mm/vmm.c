#include <mm/vmm.h>
#include <mm/pmm.h>
#include <int/isr.h>
#include <sys/stack_trace.h>

#define KERNEL_SPACE            (0x10000000)
#define KERNEL_SPACE_TABLES     (KERNEL_SPACE >> 22)

vmm_table_t* base_kernel_cr3 = 0;

vmm_table_t __attribute__((aligned(PAGE_SIZE))) _base_dir     = { 0 };
vmm_table_t __attribute__((aligned(PAGE_SIZE))) _base_table   = { 0 };

vmm_table_t __attribute__((aligned(PAGE_SIZE))) _dir     = { 0 };
vmm_table_t __attribute__((aligned(PAGE_SIZE))) _table   = { 0 };

vmm_table_t* vmm_get_cr3() {
    vmm_table_t* ret;
    asm volatile("mov %%cr3, %0;" : "=r"(ret));
    return ret;
}

vmm_table_t* vmm_replace_cr3(vmm_table_t* new) {
    vmm_table_t* old;

    if (!new)
        return NULL;

    asm volatile("mov %%cr3, %0;" : "=r"(old));

    if (old == new)
        return NULL;

    // ser_printf("Replacing cr3 from 0x%08x to 0x%08x\n", old, new);

    asm volatile("mov %0, %%cr3;" ::"r"(new) : "memory");
    return old;
}

void vmm_set_cr3(vmm_table_t* new) {
    asm volatile("mov %0, %%cr3;" ::"r"(new) : "memory");
}

void vmm_invlpg(void* new) {
    asm volatile("invlpg (%0);" ::"r"(new) : "memory");
}

vmm_table_off_t vmm_virt_to_offs(void* virt) {
    uint32_t addr = (uint32_t)virt;

    vmm_table_off_t off = {
        .tid = (addr >> 22) & 0x3FF,     // Page Directory Index
        .pid = (addr >> 12) & 0x3FF,     // Page Table Index
        .off = (addr >> 0) & 0xFFF,      // Page Offset
    };

    return off;
}

void* vmm_offs_to_virt(vmm_table_off_t offs) {
    uint32_t addr = 0;

    addr |= (offs.tid << 22) & 0xFFC00000;
    addr |= (offs.pid << 12) & 0x003FF000;
    addr |= (offs.off << 0) & 0x00000FFF;

    return (void*)addr;
}

void open_dir(vmm_table_t* page_dir) {
    void* addr = (void*)&_dir;

    vmm_table_off_t offset = vmm_virt_to_offs(addr);

    if (_base_table.entries[offset.pid].address == (uint32_t)page_dir >> 12)
        return;

    _base_table.entries[offset.pid].raw = (uint32_t)page_dir | VMM_PRESENT | VMM_WRITE | VMM_USER;
    vmm_invlpg(addr);
}

void open_table(vmm_table_t* page_table) {
    void* addr = (void*)&_table;

    vmm_table_off_t offset = vmm_virt_to_offs(addr);

    if (_base_table.entries[offset.pid].address == (uint32_t)page_table >> 12)
        return;

    _base_table.entries[offset.pid].raw = (uint32_t)page_table | VMM_PRESENT | VMM_WRITE | VMM_USER;
    vmm_invlpg(addr);
}

void close_dir() {
    void* addr = (void*)&_dir;

    vmm_table_off_t offset = vmm_virt_to_offs(addr);

    if (_base_table.entries[offset.pid].address == (uint32_t)addr >> 12)
        return;

    _base_table.entries[offset.pid].raw = (uint32_t)addr | VMM_PRESENT | VMM_WRITE | VMM_USER;
    vmm_invlpg(addr);
}

void close_table() {
    void* addr = (void*)&_table;

    vmm_table_off_t offset = vmm_virt_to_offs(addr);

    if (_base_table.entries[offset.pid].address == (uint32_t)addr >> 12)
        return;

    _base_table.entries[offset.pid].raw = (uint32_t)addr | VMM_PRESENT | VMM_WRITE | VMM_USER;
    vmm_invlpg(addr);
}

vmm_entry_t* traverse_dir_entry(vmm_table_off_t offset) {
    return (vmm_entry_t*)(&_dir.entries[offset.tid]);
}

vmm_table_t* traverse_table(vmm_table_off_t offset) {
    return (vmm_table_t*)(traverse_dir_entry(offset)->address << 12);
}

vmm_entry_t* traverse_table_entry(vmm_table_off_t offset) {
    return (vmm_entry_t*)(&_table.entries[offset.pid]);
}

void* traverse_page(vmm_table_off_t offset) {
    return (void*)(traverse_table_entry(offset)->address << 12);
}

void* virt_to_phys(void* virt, vmm_table_t* page_dir) {
    vmm_table_off_t offset = vmm_virt_to_offs(virt);
    void* phys = (void*)0xFFFFFFFF;

    open_dir(page_dir);

    vmm_entry_t* dir_entry = traverse_dir_entry(offset);
    if (dir_entry->present) {
        vmm_table_t* page_table = traverse_table(offset);
        open_table(page_table);
        vmm_entry_t* table_entry = traverse_table_entry(offset);

        if (table_entry->present)
            phys = (void*)traverse_page(offset) + offset.off;
    }

    close_table();
    close_dir();
    return phys;
}

void ensure_table(vmm_table_off_t offset) {
    vmm_entry_t* entry = traverse_dir_entry(offset);

    if (!entry->present) {
        uint32_t new_table = (uint32_t)pmm_alloc(PAGE_SIZE);

        open_table((vmm_table_t*)new_table);

        for (uint32_t i = 0; i < 1024; i++)
            _table.entries[i].raw = 0;

        close_table();

        entry->raw = new_table | VMM_PRESENT | VMM_WRITE | VMM_USER;
    }
}

uint8_t is_mapped(void* data, vmm_table_t* page_dir) {
    if (!base_kernel_cr3)
        return 0;

    return (uint32_t)virt_to_phys(data, page_dir) == 0xFFFFFFFF ? 0 : 1;
}

uint8_t range_mapped(void* data, uint32_t size) {
    uint32_t cur_addr = (uint32_t) data & VMM_4K_PERM_MASK;
    uint32_t addr_not_rounded = (uint32_t) data;
    uint32_t end_addr = (addr_not_rounded + size) & VMM_4K_PERM_MASK;
    uint32_t rounded_size = end_addr - cur_addr;
    uint32_t pages = ((rounded_size + PAGE_SIZE - 1) / PAGE_SIZE);

    for (uint32_t i = 0; i < pages; i++) {
        if (!is_mapped((void*)cur_addr, vmm_get_cr3()))
            return 0;

        cur_addr += PAGE_SIZE;
    }

    return 1;
}

int vmm_map_pages(void* phys, void* virt, vmm_table_t* page_dir, uint32_t count, uint16_t perms) {
    int ret = 0;

    uint32_t cur_virt = ((uint32_t)virt) & VMM_4K_PERM_MASK;
    uint32_t cur_phys = ((uint32_t)phys) & VMM_4K_PERM_MASK;

    open_dir(page_dir);

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        ensure_table(offset);
        vmm_table_t* page_table = traverse_table(offset);
        open_table(page_table);
        vmm_entry_t* table_entry = traverse_table_entry(offset);

        if (!table_entry->present) {
            table_entry->raw = cur_phys | perms;
            vmm_invlpg((void*)cur_virt);
        } else {
            ret = 1;
            continue;
        }

        cur_phys += PAGE_SIZE;
        cur_virt += PAGE_SIZE;
    }

    close_table();
    close_dir();

    return ret;
}

int vmm_remap_pages(void* phys, void* virt, vmm_table_t* page_dir, uint32_t count, uint16_t perms) {
    int ret = 0;

    uint32_t cur_virt = ((uint32_t)virt) & VMM_4K_PERM_MASK;
    uint32_t cur_phys = ((uint32_t)phys) & VMM_4K_PERM_MASK;

    open_dir(page_dir);

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        ensure_table(offset);
        vmm_table_t* page_table = traverse_table(offset);
        open_table(page_table);
        vmm_entry_t* table_entry = traverse_table_entry(offset);

        table_entry->raw = cur_phys | perms;
        vmm_invlpg((void*)cur_virt);
        cur_phys += PAGE_SIZE;
        cur_virt += PAGE_SIZE;
    }

    close_table();
    close_dir();

    return ret;
}

int vmm_unmap_pages(void* virt, vmm_table_t* page_dir, uint32_t count) {
    int ret = 0;

    uint32_t cur_virt = ((uint32_t)virt) & VMM_4K_PERM_MASK;

    open_dir(page_dir);

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        ensure_table(offset);
        vmm_table_t* page_table = traverse_table(offset);
        open_table(page_table);
        vmm_entry_t* table_entry = traverse_table_entry(offset);

        if (table_entry->present) {
            table_entry->raw = 0;
        } else {
            ret = 1;
        }

        vmm_invlpg((void*)cur_virt);
        cur_virt += PAGE_SIZE;
    }

    close_table();
    close_dir();

    return ret;
}

int vmm_map(void* phys, void* virt, uint32_t count, uint16_t perms) {
    return vmm_map_pages(phys, virt, vmm_get_cr3(), count, perms);
}

int vmm_remap(void* phys, void* virt, uint32_t count, uint16_t perms) {
    return vmm_remap_pages(phys, virt, vmm_get_cr3(), count, perms);
}

int vmm_unmap(void* virt, uint32_t count) {
    return vmm_unmap_pages(virt, vmm_get_cr3(), count);
}

void* vmm_virt_to_phys(void* virt) {
    return virt_to_phys(virt, vmm_get_cr3());
}

void vmm_enable_paging() {
	uint32_t cr0;
	asm volatile ("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile ("mov %0, %%cr0" : : "r" (cr0));
}

void vmm_page_fault(core_locals_t* locals) {
    core_regs_t* regs = locals->irq_regs;

    uint32_t faddr;
    asm volatile("mov %%cr2, %0" : "=r" (faddr));

    char* err = "Unknown error";

    if (!(regs->base.int_err & 0x1)) {
    	err = "Page not present";
	} else if (regs->base.int_err & 0x2) {
    	err = "Page is read-only";
	} else if (regs->base.int_err & 0x4) {
    	err = "Processor in user-mode";
    } else if (regs->base.int_err & 0x8) {
    	err = "Overwrite CPU-reserved bits";
    }

    thread_t* thread = locals->current_thread;

    ser_printf("Page fault at 0x%08x (0x%08x, %s) on core %u (tid %d, in_irq %u)\n", regs->base.eip, faddr, err, locals->core_id, thread ? thread->tid : -1, locals->in_irq);
    ser_printf("BASE CR3 0x%08x, ISR CR3 0x%08x", base_kernel_cr3, regs->base.cr3);
    if (thread)
        ser_printf(", THREAD CR3 0x%08x\n", thread->regs->base.cr3);
    else
        ser_printf("\n");

    stack_trace(16);

    if (thread)
        thread->state = THREAD_TERMINATED;

    locals->need_resched = true;
//     asm volatile("sti");

//     while (1)
//         asm volatile("hlt");
}

void vmm_memory_setup(multiboot_memory_map_t* mmap, uint32_t mmap_len) {
    uint32_t kernel_start = (uint32_t)__kernel_start;
    uint32_t kernel_end = (uint32_t)__kernel_end;

    (void)mmap;
    (void)mmap_len;

    for (uint32_t i = 0; i < 1024; i++)
        _base_dir.entries[i].raw = VMM_WRITE | VMM_USER;

    for (uint32_t i = 0; i < 1024; i++)
        _base_table.entries[i].raw = (i * PAGE_SIZE) | VMM_PRESENT | VMM_WRITE | VMM_USER;

    _base_dir.entries[0].raw = (uint32_t)&_base_table | VMM_PRESENT | VMM_WRITE | VMM_USER;

    base_kernel_cr3 = &_base_dir;

    vmm_map_pages((void*)kernel_start, (void*)kernel_start, base_kernel_cr3, ALIGN(kernel_end - kernel_start) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE | VMM_USER);

    register_int_handler(0x0E, vmm_page_fault);

    vmm_set_cr3(base_kernel_cr3);
    vmm_enable_paging();

    // for (uint32_t i = 0; i < mmap_len; i++) {
    //     if (mmap[i].type == MULTIBOOT_MEMORY_RESERVED && mmap[i].len > 0)
    //         vmm_map((void*)(uint32_t)mmap[i].addr, (void*)(uint32_t)mmap[i].addr, ALIGN((uint32_t)mmap[i].len) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE | VMM_USER);
    // }
}

void* vmm_fork_kernel_space() {
    interrupt_state_t state = interrupt_lock();

    vmm_table_t* new_dir = (vmm_table_t*)pmm_alloc(PAGE_SIZE);

    open_dir(new_dir);

    memset((uint8_t*)&_dir, 0, PAGE_SIZE);

    for (uint32_t i = 0; i < 1024; i++)
        if (base_kernel_cr3->entries[i].present)
            _dir.entries[i].raw = base_kernel_cr3->entries[i].raw;

    close_dir();

    interrupt_unlock(state);

    return (void*)new_dir;
}

void vmm_free_cr3(vmm_table_t* table0) {
    if (!table0 || table0 == base_kernel_cr3)
        return;

    interrupt_state_t state = interrupt_lock();

    open_dir(table0);

    vmm_table_off_t offset = { .tid = 0, .pid = 0, .off = 0 };

    for (; offset.tid < 1024; offset.tid++) {
        vmm_entry_t* dir_entry = traverse_dir_entry(offset);
        vmm_entry_t* dir_entry_base = &base_kernel_cr3->entries[offset.tid];

        if (dir_entry->present && dir_entry->raw != dir_entry_base->raw) {
            vmm_table_t* table = traverse_table(offset);
            vmm_table_t* table_base = (vmm_table_t*)(base_kernel_cr3->entries[offset.tid].address << 12);

            open_table(table);

            for (; offset.pid < 1024; offset.pid++) {
                vmm_entry_t* table_entry = traverse_table_entry(offset);
                vmm_entry_t* table_entry_base = &table_base->entries[offset.pid];

                if (table_entry->present && table_entry->raw != table_entry_base->raw) {
                    void* page = traverse_page(offset);

                    uint32_t virt = (uint32_t)vmm_offs_to_virt(offset);
                    ser_printf("Freeing page VIRT 0x%08x - PHYS 0x%08x\n", virt, page);
                    pmm_unalloc(page, PAGE_SIZE);
                }
            }
        }
    }

    close_table();
    close_dir();

    pmm_unalloc(table0, PAGE_SIZE);

    interrupt_unlock(state);
}
