#include <mm/vmm.h>
#include <mm/pmm.h>
#include <int/isr.h>

vmm_table_t* base_kernel_cr3 = 0;

vmm_table_t* vmm_get_cr3() {
    vmm_table_t* ret;
    asm volatile("mov %%cr3, %0;" : "=r"(ret));
    return ret;
}

void vmm_set_cr3(vmm_table_t* new) {
    asm volatile("mov %0, %%cr3;" ::"r"(new) : "memory");
}

void vmm_invlpg(uint32_t new) {
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

vmm_table_t* traverse_table(vmm_table_t* page_dir, vmm_table_off_t offset) {
    return (vmm_table_t*)(page_dir->entries[offset.tid].address << 12);
}

void* traverse_page(vmm_table_t* page_table, vmm_table_off_t offset) {
    return (void*)(page_table->entries[offset.pid].address << 12);
}

void* virt_to_phys(void* virt, vmm_table_t* page_dir) {
    vmm_table_off_t offset = vmm_virt_to_offs(virt);

    if (page_dir->entries[offset.tid].present) {
        vmm_table_t* page_table = traverse_table(page_dir, offset);

        if (page_table->entries[offset.pid].present)
            return (void*)traverse_page(page_table, offset) + offset.off;
    }

    return (void*)0xFFFFFFFF;
}

void vmm_ensure_table(vmm_table_t* page_dir, vmm_table_off_t offset) {
    if (!page_dir->entries[offset.tid].present) {
        uint32_t new_table = (uint32_t)pmm_alloc(PAGE_SIZE);

        memset((uint8_t*)new_table, 0, PAGE_SIZE);
        page_dir->entries[offset.tid].raw = new_table | VMM_PRESENT | VMM_WRITE | VMM_USER;
    }
}

uint8_t is_mapped(void* data, vmm_table_t* page_dir) {
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

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        vmm_ensure_table(page_dir, offset);
        vmm_table_t* page_table = traverse_table(page_dir, offset);
        vmm_entry_t* page = &page_table->entries[offset.pid];

        if (!page->present) {
            page->raw = cur_phys | (perms | VMM_PRESENT);
            vmm_invlpg(cur_virt);
            cur_phys += PAGE_SIZE;
            cur_virt += PAGE_SIZE;
        } else {
            ret = 1;
            cur_phys += PAGE_SIZE;
            cur_virt += PAGE_SIZE;
            continue;
        }
    }

    return ret;
}

int vmm_remap_pages(void* phys, void* virt, vmm_table_t* page_dir, uint32_t count, uint16_t perms) {
    int ret = 0;

    uint32_t cur_virt = ((uint32_t)virt) & VMM_4K_PERM_MASK;
    uint32_t cur_phys = ((uint32_t)phys) & VMM_4K_PERM_MASK;

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        vmm_ensure_table(page_dir, offset);
        vmm_table_t* page_table = traverse_table(page_dir, offset);
        vmm_entry_t* page = &page_table->entries[offset.pid];

        page->raw = cur_phys | (perms | VMM_PRESENT);
        vmm_invlpg(cur_virt);
        cur_phys += PAGE_SIZE;
        cur_virt += PAGE_SIZE;
    }

    return ret;
}

int vmm_unmap_pages(void* virt, vmm_table_t* page_dir, uint32_t count) {
    int ret = 0;

    uint32_t cur_virt = ((uint32_t)virt) & VMM_4K_PERM_MASK;

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offset = vmm_virt_to_offs((void*)cur_virt);

        vmm_ensure_table(page_dir, offset);
        vmm_table_t* page_table = traverse_table(page_dir, offset);
        vmm_entry_t* page = &page_table->entries[offset.pid];

        if (!page->present) {
            page->raw = 0;
        } else {
            ret = 1;
        }

        vmm_invlpg(cur_virt);
        cur_virt += PAGE_SIZE;
    }

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
    core_regs_t* regs = &locals->irq_regs;

    uint32_t faddr;
    asm volatile("mov %%cr2, %0" : "=r" (faddr));

    char* err = "Unknown error";

    if (!(regs->int_err & 0x1)) {
    	err = "Page not present";
	} else if (regs->int_err & 0x2) {
    	err = "Page is read-only";
	} else if (regs->int_err & 0x4) {
    	err = "Processor in user-mode";
    } else if (regs->int_err & 0x8) {
    	err = "Overwrite CPU-reserved bits";
    }

    thread_t* thread = locals->current_thread;
    thread->state = THREAD_TERMINATED;

    isprintf("Page fault at 0x%08x (0x%08x, %s) on core %u (tid %d, in_irq %u)\n", regs->eip, faddr, err, locals->core_index, thread ? thread->tid : -1, locals->in_irq);
    // print_stacktrace(&locals->irq_regs);
    asm volatile("sti");

    while (1)
        asm volatile("hlt");
}

void vmm_memory_setup(multiboot_mmap_t* mmap, uint32_t mmap_len) {
    uint32_t kernel_start = (uint32_t)__kernel_start;
    uint32_t kernel_end = (uint32_t)__kernel_end;

    vmm_table_t* page_dir = (vmm_table_t*)pmm_alloc(PAGE_SIZE);
    memset((uint8_t*)page_dir, 0, PAGE_SIZE);

    // vmm_map_pages((void*)0xB8000, (void*)0xB8000, page_dir, 1, VMM_PRESENT | VMM_WRITE);
    // vmm_map_pages((void*)kernel_start, (void*)kernel_start, page_dir, ALIGN(kernel_end - kernel_start) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

    vmm_map_pages((void*)0, (void*)0, page_dir, ALIGN(kernel_end) / PAGE_SIZE, VMM_PRESENT | VMM_WRITE);

    vmm_set_cr3(page_dir);

    register_int_handler(0x0E, vmm_page_fault);

    vmm_enable_paging();

    base_kernel_cr3 = vmm_get_cr3();
}

void* vmm_fork_kernel_space() {
    interrupt_state_t state = interrupt_lock();

    vmm_table_t* table0old = (vmm_table_t*)base_kernel_cr3;
    vmm_table_t* table0new = (vmm_table_t*)pmm_alloc(PAGE_SIZE);

    memset((uint8_t*)table0new, 0, PAGE_SIZE);

    for (uint32_t i = 0; i < 8; i++)
        table0new->entries[i] = table0old->entries[i];

    interrupt_unlock(state);

    return (void*)table0new;
}