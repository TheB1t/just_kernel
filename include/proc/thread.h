#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <proc/proc.h>

#define THREAD_HEAD(x)          (&(x)->gt_head)
#define THREAD_LIST(x)          (&(x)->gt_list)

#define PROC_THREAD_HEAD(x)     (&(x)->pt_head)
#define PROC_THREAD_LIST(x)     (&(x)->pt_list)

#define for_each_thread(pos, list)          \
    for (pos = (THREAD_HEAD(&list))->next;  \
        pos != (THREAD_HEAD(&list));        \
        pos = pos->next)                    \

#define for_each_process_thread(pos, process)       \
    for (pos = (PROC_THREAD_HEAD(process))->next;   \
        pos != (PROC_THREAD_HEAD(process));         \
        pos = pos->next)                            \

#define entry_thread(iter)          list_entry(iter, thread_t, gt_list)
#define entry_process_thread(iter)  list_entry(iter, thread_t, pt_list)

typedef struct {
    uint32_t    eax;
    uint32_t    ebx;
    uint32_t    ecx;
    uint32_t    edx;
    uint32_t    ebp;
    uint32_t    edi;
    uint32_t    esi;

    uint32_t    esp;
    uint32_t    eip;

    uint32_t    ss;
    uint32_t    cs;
    uint32_t    fs;
    uint32_t    eflags;

    uint32_t    cr3;
    uint32_t    mxcsr;
    uint16_t    fcw;
} thread_regs_t;

typedef struct {
    struct list_head gt_head;
} thread_list_t;

typedef enum {
    THREAD_STOPPED,
    THREAD_RUNNING,
    THREAD_TERMINATED,

    _TASK_LAST,
    TASK_MAX = _TASK_LAST
} thread_state_t;

typedef struct thread {
    struct list_head pt_list;
    struct list_head gt_list;

    uint32_t    stack;

    int32_t     tid;
    process_t*  parent;

    int32_t     cpu;
    uint8_t     ring;

    thread_state_t  state;
    thread_regs_t   regs;

    uint32_t    tsc_started;   // The last time this task was started
    uint32_t    tsc_stopped;   // The last time this task was stopped
    uint32_t    tsc_total;     // The total time this task has been running for

    char        sse_region[512] __attribute__((aligned(16)));
} thread_t;

thread_t* thread_create(process_t* proc, void* entry);