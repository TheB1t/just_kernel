#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>

#define PROC_HEAD(x)   (&(x)->gp_head)
#define PROC_LIST(x)   (&(x)->gp_list)

#define get_main_thread(proc)           entry_process_thread(PROC_THREAD_HEAD(proc)->next)

#define for_each_process(pos, list)         \
    for (pos = (PROC_HEAD(&list))->next;    \
        pos != (PROC_HEAD(&list));          \
        pos = pos->next)                    \

typedef struct {
    struct list_head gp_head;
} process_list_t;

typedef struct process {
    char            name[64];

    uint32_t        cr3;
    uint8_t         ring;

    struct list_head gp_list;
    struct list_head pt_head;
} process_t;

process_t*  proc_create(const char* name, void* entry, uint8_t ring);