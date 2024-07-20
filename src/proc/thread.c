#include <proc/thread.h>
#include <int/dt.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

static uint32_t next_tid = 0;

thread_regs_t default_kernel_regs = {
    .eax=0, .ebx=0, .ecx=0, .edx=0, .ebp=0,
    .edi=0, .esi=0, .esp=0, .eip=0,
    .ss=DESC_SEG(DESC_KERNEL_DATA, PL_RING0),
    .cs=DESC_SEG(DESC_KERNEL_CODE, PL_RING0),
    .fs=0, .eflags=0x202,
    .cr3=0, .mxcsr=0x1F80, .fcw=0x33F
};

thread_regs_t default_user_regs = {
    .eax=0, .ebx=0, .ecx=0, .edx=0, .ebp=0,
    .edi=0, .esi=0, .esp=0, .eip=0,
    .ss=DESC_SEG(DESC_USER_DATA, PL_RING3),
    .cs=DESC_SEG(DESC_USER_CODE, PL_RING3),
    .fs=0, .eflags=0x202,
    .cr3=0, .mxcsr=0x1F80, .fcw=0x33F
};

extern symbol load_thread;
extern symbol load_thread_end;

thread_t* thread_create(process_t* proc, void* entry) {
    sched_lock();

    thread_t* thread = (thread_t*)kmalloc(sizeof(thread_t));

    if (!thread)
        goto done;

    memset((uint8_t*)thread, 0, sizeof(thread_t));

    if (proc->ring == 3) {
        thread->regs        = default_user_regs;

        void* stack         = pmm_alloc(PAGE_SIZE);
        vmm_map_pages(stack, (void*)0x50000000, (void*)proc->cr3, 1, VMM_PRESENT | VMM_WRITE | VMM_USER);
        thread->stack       = 0x50000000 + PAGE_SIZE;
    } else {
        thread->regs        = default_kernel_regs;
        thread->stack       = (uint32_t)(kmalloc(PAGE_SIZE) + PAGE_SIZE);
    }

    thread->tid         = next_tid++;
    thread->state       = THREAD_STOPPED;
    thread->parent      = proc;
    thread->cpu         = -1;
    thread->ring        = proc->ring;
    thread->regs.cr3    = proc->cr3;
    thread->regs.eax    = (uint32_t)entry;
    thread->regs.eip    = (uint32_t)load_thread;
    thread->regs.esp    = (uint32_t)thread->stack;

    add_thread2proc(proc, thread);

done:
    sched_unlock();
    return thread;
}