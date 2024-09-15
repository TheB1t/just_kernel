#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <mm/vmm.h>
#include <mm/heap.h>
#include <fs/elf/elf.h>

#define PROC_HEAD(x)   (&(x)->gp_head)
#define PROC_LIST(x)   (&(x)->gp_list)

#define get_main_thread(proc)       entry_process_thread(PROC_THREAD_HEAD(proc)->next)

#define for_each_process(pos, list)         \
    for (pos = (PROC_HEAD(&list))->next;    \
        pos != (PROC_HEAD(&list));          \
        pos = pos->next)                    \

#define entry_process(iter)         list_entry(iter, process_t, gp_list)

typedef struct {
    struct list_head gp_head;
} process_list_t;

typedef struct process {
    char            name[64];

    vmm_table_t*    cr3;
    uint8_t         ring;
    Heap_t*         heap;

    ELF32Obj_t*     elf_obj;

    struct list_head gp_list;
    struct list_head pt_head;
} process_t;

process_t*  proc_create_kernel(const char* name, uint8_t ring);
process_t*  proc_create_from_elf(const char* name, ELF32Obj_t* hdr, uint8_t ring);
void proc_free(process_t* proc);