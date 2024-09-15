#include <klibc/stdlib.h>
#include <mm/heap.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

#include <sys/smp.h>
#include <sys/stack_trace.h>

#include <proc/sched.h>

extern Heap_t* kernel_heap;
extern lock_t kprintf_lock;
extern lock_t sprintf_lock;

void* kmalloc(uint32_t size) {
    return (void*)heap_malloc(kernel_heap, size, 0);
}

void* kmalloc_a(uint32_t size) {
    return (void*)heap_malloc(kernel_heap, size, 1);
}

void kfree(void* addr) {
    heap_free(kernel_heap, addr);
}

void *kcalloc(uint32_t size) {
    void *buffer = kmalloc(size);

    memset((uint8_t*)buffer, 0, size);
    return buffer;
}

void panic(char *msg) {
    asm volatile("cli");

    static uint8_t panic_counter = 0;

    if (panic_counter++ == 0) {
        unlock(kprintf_lock);
        unlock(sprintf_lock);

        kprintf("Kernel PANIC!!! [%s]\n", msg);

        core_locals_t* locals = get_core_locals();
        if (locals && locals->in_irq) {
            core_regs_t* regs = locals->irq_regs;

            ser_printf("EAX: 0x%08x, EBX: 0x%08x, ECX 0x%08x\n", regs->eax, regs->ebx, regs->ecx);
            ser_printf("EDX: 0x%08x, ESI: 0x%08x, EDI 0x%08x\n", regs->edx, regs->esi, regs->edi);
            ser_printf("EBP: 0x%08x, ESP: 0x%08x, EIP 0x%08x\n", regs->ebp, regs->esp, regs->eip);
            ser_printf("EFLAGS: 0x%08x\n", regs->eflags);
            ser_printf("CS: 0x%04x, DS: 0x%04x\n", regs->cs, regs->ds);
            ser_printf("User SS: 0x%04x, User ESP: 0x%08x\n", regs->user_ss, regs->user_esp);
        } else {
            ser_printf("Can't get panic context\n");
        }

        stack_trace(16);

        sched_panic();
    } else if (panic_counter++ == 1) {
        ser_printf("DOUBLE PANIC!!!\n");
    } // Otherwise just halt, because we falling in ser_printf

    while (1)
        asm volatile("hlt");
}
