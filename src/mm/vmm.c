#include <mm/vmm.h>
#include <mm/pmm.h>
#include <klibc/lock.h>

lock_t vmm_lock = INIT_LOCK(vmm_lock);

uint32_t base_kernel_cr3 = 0;

uint32_t vmm_get_base() {
    uint32_t ret;
    asm volatile("mov %%cr3, %0;" : "=r"(ret));
    return ret;
}

void vmm_set_base(uint32_t new) {
    asm volatile("mov %0, %%cr3;" ::"r"(new) : "memory");
}

void vmm_invlpg(uint32_t new) {
    asm volatile("invlpg (%0);" ::"r"(new) : "memory");
}

vmm_table_off_t vmm_virt_to_offs(void* virt) {
    uint32_t addr = (uint32_t)virt;

    vmm_table_off_t off = {
        .p0_off = (addr >> 22) & 0x3FF,
        .p1_off = (addr >> 12) & 0x3FF
    };

    return off;
}

void* vmm_offs_to_virt(vmm_table_off_t offs) {
    uint32_t addr = 0;

    addr |= (offs.p0_off << 22) & 0xFFC00000;
    addr |= (offs.p1_off << 12) & 0x003FF000;

    return (void*)addr;
}

uint32_t get_entry(vmm_table_t* table, uint32_t offset) {
    return table->entries[offset];
}

vmm_table_t* traverse_table(vmm_table_t* table, uint32_t offset) {
    return (vmm_table_t*)(get_entry(table, offset) & VMM_4K_PERM_MASK);
}

void* traverse_page(vmm_table_t* table, uint32_t offset) {
    return (void*)(get_entry(table, offset) & VMM_4K_PERM_MASK);
}

void* virt_to_phys(void* virt, vmm_table_t* table0) {
    uint32_t page4k_offset = ((uint32_t)virt) & 0xfff;
    vmm_table_off_t offs = vmm_virt_to_offs(virt);

    if (get_entry(table0, offs.p0_off) & VMM_PRESENT) {
        vmm_table_t* table1 = traverse_table(table0, offs.p0_off);

        if (get_entry(table1, offs.p1_off) & VMM_PRESENT)
            return (void*)traverse_page(table1, offs.p1_off) + page4k_offset;
    }

    return (void*)0xFFFFFFFF;
}

void vmm_ensure_table(vmm_table_t* table, uint16_t offset) {
    if (!(table->entries[offset] & VMM_PRESENT)) {
        uint32_t new_table = (uint32_t)pmm_alloc(PAGE_SIZE);

        memset((uint8_t*)new_table, 0, PAGE_SIZE);
        table->entries[offset] = new_table | VMM_PRESENT | VMM_WRITE | VMM_USER;
    }
}

uint8_t is_mapped(void* data, void* cr3) {
    uint32_t phys_addr = (uint32_t)virt_to_phys(data, (vmm_table_t*)cr3);

    return phys_addr == 0xFFFFFFFF ? 0 : 1;
}

uint8_t range_mapped(void* data, uint32_t size) {
    uint32_t cur_addr = (uint32_t) data & ~(0xfff);
    uint32_t addr_not_rounded = (uint32_t) data;
    uint32_t end_addr = (addr_not_rounded + size) & ~(0xfff);
    uint32_t rounded_size = end_addr - cur_addr;
    uint32_t pages = ((rounded_size + PAGE_SIZE - 1) / PAGE_SIZE);

    for (uint32_t i = 0; i < pages; i++) {
        if (!is_mapped((void*) cur_addr, (vmm_table_t*)vmm_get_base()))
            return 0;

        cur_addr += PAGE_SIZE;
    }

    return 1;
}

vmm_table_t* vmm_get_table(vmm_table_off_t* offs, vmm_table_t* table) {
    vmm_ensure_table(table, offs->p0_off);
    return traverse_table(table, offs->p0_off);
}

int vmm_map_pages(void* phys, void* virt, void* cr3, uint32_t count, uint16_t perms) {
    interrupt_state_t state = interrupt_lock();

    lock(vmm_lock);

    int ret = 0;
    vmm_table_t* table0 = (vmm_table_t*)cr3;

    uint8_t* cur_virt = (uint8_t *) (((uint32_t)virt) & VMM_4K_PERM_MASK);
    uint32_t cur_phys = ((uint32_t) phys) & VMM_4K_PERM_MASK;

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offs = vmm_virt_to_offs((void*)cur_virt);
        vmm_table_t* table1 = vmm_get_table(&offs, table0);

        if (!(table1->entries[offs.p1_off] & VMM_PRESENT)) {
            table1->entries[offs.p1_off] = cur_phys | perms;
            vmm_invlpg((uint32_t)cur_virt);
            cur_phys += PAGE_SIZE;
            cur_virt += PAGE_SIZE;
        } else {
            ret = 1;
            cur_phys += PAGE_SIZE;
            cur_virt += PAGE_SIZE;
            continue;
        }
    }

    unlock(vmm_lock);
    interrupt_unlock(state);

    return ret;
}

int vmm_remap_pages(void* phys, void* virt, void* cr3, uint32_t count, uint16_t perms) {
    interrupt_state_t state = interrupt_lock();
    lock(vmm_lock);

    int ret = 0;
    vmm_table_t* table0 = (vmm_table_t*)cr3;

    uint8_t *cur_virt = (uint8_t *) (((uint32_t) virt) & VMM_4K_PERM_MASK);
    uint32_t cur_phys = ((uint32_t) phys) & VMM_4K_PERM_MASK;

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offs = vmm_virt_to_offs((void*)cur_virt);
        vmm_table_t* table1 = vmm_get_table(&offs, table0);

        table1->entries[offs.p1_off] = cur_phys | (perms | VMM_PRESENT);

        vmm_invlpg((uint32_t)cur_virt);
        cur_phys += PAGE_SIZE;
        cur_virt += PAGE_SIZE;
    }

    unlock(vmm_lock);
    interrupt_unlock(state);

    return ret;
}

int vmm_unmap_pages(void* virt, void* cr3, uint32_t count) {
    interrupt_state_t state = interrupt_lock();
    lock(vmm_lock);

    int ret = 0;
    vmm_table_t* table0 = (vmm_table_t*)cr3;
    uint32_t cur_virt = (uint32_t)virt;

    for (uint32_t page = 0; page < count; page++) {
        vmm_table_off_t offs = vmm_virt_to_offs((void*)cur_virt);
        vmm_table_t* table1 = vmm_get_table(&offs, table0);

        if ((table1->entries[offs.p1_off] & VMM_PRESENT)) {
            table1->entries[offs.p1_off] = 0;
        } else {
            ret = 1;
        }

        vmm_invlpg(cur_virt);
        cur_virt += PAGE_SIZE;
    }

    unlock(vmm_lock);
    interrupt_unlock(state);

    return ret;
}

int vmm_map(void* phys, void* virt, uint32_t count, uint16_t perms) {
    return vmm_map_pages(phys, virt, (void*)vmm_get_base(), count, perms);
}

int vmm_remap(void* phys, void* virt, uint32_t count, uint16_t perms) {
    return vmm_remap_pages(phys, virt, (void*)vmm_get_base(), count, perms);
}

int vmm_unmap(void* virt, uint32_t count) {
    return vmm_unmap_pages(virt, (void*)vmm_get_base(), count);
}

void* vmm_virt_to_phys(void* virt) {
    return virt_to_phys(virt, (void*)vmm_get_base());
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
    asm volatile ("mov %%cr2, %0" : "=r" (faddr));

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

void vmm_memory_setup(uint32_t kernel_start, uint32_t kernel_len) {
    uint32_t kernel_start_page = kernel_start / PAGE_SIZE;
    uint32_t kernel_pages = (kernel_len + PAGE_SIZE - 1) / PAGE_SIZE;

    void* new_cr3 = pmm_alloc(PAGE_SIZE);
    memset(new_cr3, 0, PAGE_SIZE);

    vmm_map_pages((void*)0, (void*)0, new_cr3, kernel_start_page + kernel_pages, VMM_PRESENT | VMM_WRITE);
    vmm_set_base((uint32_t)new_cr3);

    register_int_handler(14, vmm_page_fault);

    vmm_enable_paging();

    base_kernel_cr3 = vmm_get_base();
}

void* vmm_fork_kernel_space(void* cr3) {
    interrupt_state_t state = interrupt_lock();
    lock(vmm_lock);

    vmm_table_t* table0old = (vmm_table_t*)cr3;
    vmm_table_t* table0new = (vmm_table_t*)pmm_alloc(PAGE_SIZE);

    memset((uint8_t*)table0new, 0, PAGE_SIZE);

    for (uint32_t i = 0; i < 8; i++)
        table0new->entries[i] = table0old->entries[i];

    unlock(vmm_lock);
    interrupt_unlock(state);

    return (void*)table0new;
}