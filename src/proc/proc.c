#include <proc/proc.h>
#include <proc/thread.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <klibc/string.h>
#include <sys/stack_trace.h>

extern vmm_table_t* base_kernel_cr3;
extern Heap_t* kernel_heap;

process_t* proc_alloc(const char* name, uint8_t ring) {
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));

    if (proc) {
        memset((uint8_t*)proc, 0, sizeof(process_t));
        strncpy((char*)name, proc->name, 64);

        INIT_LIST_HEAD(PROC_THREAD_HEAD(proc));

        if (ring == 3) {
            proc->heap = (Heap_t*)kmalloc(sizeof(Heap_t));
            proc->cr3 = vmm_fork_kernel_space();

            vmm_table_t* old_cr3 = vmm_replace_cr3(proc->cr3);
            createHeap((uint32_t)proc->heap, (uint32_t)proc->cr3, 0x50000000, HEAP_MIN_SIZE, 0x50000000 + (HEAP_MIN_SIZE * 2), VMM_WRITE | VMM_USER);
            vmm_replace_cr3(old_cr3);
        } else {
            proc->cr3 = base_kernel_cr3;
            proc->heap = kernel_heap;
        }

        proc->ring = ring;
    }

    return proc;
}

void proc_free(process_t* proc) {
    if (proc) {
        struct list_head* iter;
        for_each_process_thread(iter, proc) {
            thread_t* thread = entry_process_thread(iter);
            thread_free(thread);
        }

        vmm_free_cr3(proc->cr3);

        kfree(proc);
    }
}

process_t* proc_create_kernel(const char* name, uint8_t ring) {
    process_t* proc = proc_alloc(name, ring);

    if (proc)
        proc->elf_obj = KERNEL_TABLE_OBJ;

    return proc;
}

process_t* proc_create_from_elf(const char* name, ELF32Obj_t* hdr, uint8_t ring) {
    process_t* proc = proc_alloc(name, ring);

    if (proc) {
        proc->elf_obj = hdr;

        for (uint32_t i = 0; i < ELF32_PROGTAB_NENTRIES(hdr); i++) {

            ELF32ProgramHeader_t* ph = ELF32_PROGRAM(hdr, i);
            if (ph->type == PT_LOAD) {
                uint32_t pages = ALIGN(ph->memsz) / PAGE_SIZE;
                void* paddr = pmm_alloc(ph->memsz);

                memset(paddr, 0, ph->memsz);
                memcpy(((uint8_t*)hdr->header) + ph->offset, (uint8_t*)paddr, ph->memsz);

                vmm_map_pages(paddr, (void*)ph->vaddr, (vmm_table_t*)proc->cr3, pages, VMM_PRESENT | VMM_WRITE | VMM_USER);
            }
        }
    }

    return proc;
}