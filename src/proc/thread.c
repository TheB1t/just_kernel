#include <proc/thread.h>
#include <proc/sched.h>
#include <int/dt.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/smp.h>

static uint32_t next_tid = 0;

thread_t* thread_alloc() {
    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t));

    if (thread) {
        memset((uint8_t*)thread, 0, sizeof(thread_t));
        thread->kernel_stack    = (uint32_t)(kmalloc_a(PAGE_SIZE) + PAGE_SIZE);
        thread->regs            = (core_regs_t*)(thread->kernel_stack - sizeof(core_regs_t));
        thread->sse_region      = (sse_region_t*)kmalloc_a(sizeof(sse_region_t));
        memset((uint8_t*)thread->regs, 0, sizeof(core_regs_t));
        memset((uint8_t*)thread->sse_region, 0, sizeof(sse_region_t));
    }

    return thread;
}

void thread_free(thread_t* thread) {
    if (thread) {
        if (thread->user_stack) {
            thread->user_stack -= PAGE_SIZE;

            vmm_table_t* old_cr3 = vmm_replace_cr3(thread->parent->cr3);
            heap_free(thread->parent->heap, (void*)thread->user_stack);
            vmm_replace_cr3(old_cr3);
        }

        kfree((void*)(thread->kernel_stack - PAGE_SIZE));
        kfree((void*)thread);
    }
}

thread_t* thread_create(process_t* proc, void* entry) {
    sched_lock();

    thread_t* thread = thread_alloc();

    if (thread) {
        if (proc->ring == 3) {
            vmm_table_t* old_cr3 = vmm_replace_cr3(proc->cr3);
            thread->user_stack      = heap_malloc(proc->heap, PAGE_SIZE, 1) + PAGE_SIZE;
            vmm_replace_cr3(old_cr3);

            thread->regs->cs        = DESC_SEG(DESC_USER_CODE, PL_RING3);
            thread->regs->ds        = DESC_SEG(DESC_USER_DATA, PL_RING3);
            thread->regs->user_ss   = thread->regs->ds;
            thread->regs->user_esp  = (uint32_t)thread->user_stack;
        } else {
            thread->user_stack      = 0;

            thread->regs->cs        = DESC_SEG(DESC_KERNEL_CODE, PL_RING0);
            thread->regs->ds        = DESC_SEG(DESC_KERNEL_DATA, PL_RING0);
        }

        thread->tid             = next_tid++;
        thread->state           = THREAD_STOPPED;
        thread->parent          = proc;
        thread->cpu             = -1;
        thread->regs->eip       = (uint32_t)entry;
        thread->regs->cr3       = (uint32_t)proc->cr3;
        thread->regs->esp       = (uint32_t)thread->kernel_stack;
        thread->regs->eflags    = 0x202;

        add_thread2proc(proc, thread);
    }

    sched_unlock();
    return thread;
}