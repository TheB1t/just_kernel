#pragma once

#include <klibc/stdlib.h>
#include <klibc/llist.h>
#include <klibc/lock.h>
#include <proc/proc.h>
#include <proc/thread.h>

#define add_thread2proc(proc, thread)   list_add_tail(PROC_THREAD_LIST(thread), PROC_THREAD_HEAD(proc))
#define del_thread2proc(thread)         list_del(PROC_THREAD_LIST(thread))

#define proc_have_thread(proc)          (!list_empty(PROC_THREAD_HEAD(proc)))

extern process_t* kernel_proc;

void sched_lock();
void sched_unlock();

void sched_panic();

void sched_run_thread(thread_t* thread);
void sched_run_proc(process_t* proc);

void sched_init_core();
void sched_init(void* entry);
void sched_run();