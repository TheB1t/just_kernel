#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <sys/core_regs.h>
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
    struct list_head gt_head;
} thread_list_t;

typedef enum {
    THREAD_STOPPED,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_TERMINATED,

    _TASK_LAST,
    TASK_MAX = _TASK_LAST
} thread_state_t;

typedef struct thread {
    core_regs_t*    regs;
    thread_state_t  state;

    int32_t     cpu;
    int32_t     tid;
    process_t*  parent;

    uint32_t    user_stack;
    uint32_t    kernel_stack;
    struct {
        uint32_t    v86_if : 1;
        uint32_t    res    : 31;
    } flags;

    uint32_t    sleep_time;
    uint32_t    exitcode;
    uint32_t    priority;

    uint32_t    tsc_started;   // The last time this task was started
    uint32_t    tsc_stopped;   // The last time this task was stopped
    uint32_t    tsc_total;     // The total time this task has been running for

    struct list_head pt_list;
    struct list_head gt_list;

    // sse_region_t* sse_region;
} thread_t;

thread_t* thread_create(process_t* proc, void* entry);
void thread_free(thread_t* thread);

extern void _idle();