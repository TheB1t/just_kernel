#include <klibc/stdlib.h>
#include <mm/heap.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

#include <sys/smp.h>

#include <proc/sched.h>

extern Heap_t* kernel_heap;
extern lock_t kprintf_lock;
extern lock_t sprintf_lock;

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

void print_stacktrace(core_regs_t* regs) {
  uint32_t* ebp = (uint32_t*)regs->ebp;

  sprintf("Stack trace: \n");
  sprintf("    0x%08x\n", regs->eip);
  while (ebp) {
    uint32_t eip = ebp[1];

    sprintf("    0x%08x\n", eip);
    ebp = (uint32_t*)ebp[0];
  }
}

void panic(char *msg) {
    asm volatile("cli");
    static uint8_t panic_counter = 0;

    if (panic_counter++ == 0) {
        unlock(kprintf_lock);
        unlock(sprintf_lock);

        sched_panic();

        kprintf("Kernel PANIC!!! [%s]\n", msg);

        isprintf("eax = 0x%08x\n", get_core_locals()->irq_regs.eax);
        isprintf("ebx = 0x%08x\n", get_core_locals()->irq_regs.ebx);
        isprintf("ecx = 0x%08x\n", get_core_locals()->irq_regs.ecx);
        isprintf("edx = 0x%08x\n", get_core_locals()->irq_regs.edx);
        isprintf("esi = 0x%08x\n", get_core_locals()->irq_regs.esi);
        isprintf("edi = 0x%08x\n", get_core_locals()->irq_regs.edi);
        isprintf("ebp = 0x%08x\n", get_core_locals()->irq_regs.ebp);
        isprintf("esp = 0x%08x\n", get_core_locals()->irq_regs.esp);
        isprintf("eip = 0x%08x\n", get_core_locals()->irq_regs.eip);
        isprintf("cs = 0x%04x\n", get_core_locals()->irq_regs.cs);
        isprintf("ss = 0x%04x\n", get_core_locals()->irq_regs.ss);
        isprintf("ds = 0x%04x\n", get_core_locals()->irq_regs.ds);
        isprintf("eflags = 0x%08x\n", get_core_locals()->irq_regs.eflags);

        print_stacktrace(&get_core_locals()->irq_regs);
    } else if (panic_counter++ == 1) {
        kprintf("DOUBLE PANIC!!!\n");
    } // Otherwise just halt, because we falling in kprintf

    while (1)
        asm volatile("hlt");
}
