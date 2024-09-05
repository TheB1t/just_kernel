#include <proc/proc.h>
#include <proc/thread.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <klibc/string.h>

extern uint32_t base_kernel_cr3;

process_t* proc_create(const char* name, void* entry, uint8_t ring) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));

    if (!proc)
        goto done;

    memset((uint8_t*)proc, 0, sizeof(process_t));

    strncpy((char*)name, proc->name, 64);

    INIT_LIST_HEAD(PROC_THREAD_HEAD(proc));

    if (ring == 3) {
        proc->cr3 = (uint32_t)vmm_fork_kernel_space();
    } else {
        proc->cr3 = base_kernel_cr3;
    }

    proc->ring = ring;

    thread_create(proc, entry);

done:
    return proc;
}