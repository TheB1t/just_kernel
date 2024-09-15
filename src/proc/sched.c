#include <proc/sched.h>
#include <proc/syscalls.h>
#include <drivers/pit.h>
#include <int/isr.h>
#include <sys/smp.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <klibc/lock.h>

#define add_thread(thread)      list_add_tail(THREAD_LIST(thread), THREAD_HEAD(&thread_list))
#define del_thread(thread)      list_del_init(THREAD_LIST(thread))

#define add_proc(proc)          list_add_tail(PROC_LIST(proc), PROC_HEAD(&proc_list))
#define del_proc(proc)          list_del_init(PROC_LIST(proc))

#define have_thread()           (!list_empty(THREAD_HEAD(&thread_list)))
#define have_proc()             (!list_empty(PROC_HEAD(&proc_list)))

extern symbol stack;

process_t*      kernel_proc = 0;
process_t*      idle_proc   = 0;

process_list_t  proc_list   = {0};
thread_list_t   thread_list = {0};

lock_t          _sched_lock = INIT_LOCK(_sched_lock);

void sched_lock() {
    lock(_sched_lock);
}

void sched_unlock() {
    unlock(_sched_lock);
}

void sched_panic() {
    ser_printf("[SCHED] Core %u in panic (cause)!\n", get_core_locals()->core_id);

    smp_send_global_interrupt(0xF1);
}

void remove_terminated_threads() {
    struct list_head* iter;

    for_each_thread(iter, thread_list) {
        thread_t* thread = entry_thread(iter);

        assert(thread != NULL);

        if (thread->state == THREAD_TERMINATED) {
            del_thread(thread);
            thread_free(thread);
        }
    }
}

void remove_terminated_processes() {
    struct list_head* iter;

    for_each_process(iter, proc_list) {
        process_t* proc = entry_process(iter);

        assert(proc != NULL);

        if (proc_have_thread(proc))
            continue;

        del_proc(proc);
    }
}

void update_sleeping_threads(core_locals_t* locals) {
    if (locals->core_id != 0)
        return;

    struct list_head* iter;

    for_each_thread(iter, thread_list) {
        thread_t* thread = entry_thread(iter);

        if (thread->state == THREAD_SLEEPING) {
            thread->sleep_time -= PIT_SPEED_US > thread->sleep_time ? thread->sleep_time : PIT_SPEED_US;

            if (!thread->sleep_time)
                thread->state = THREAD_STOPPED;
        }
    }
}

thread_t* pick_thread(core_locals_t* locals) {
    struct list_head* iter;

    thread_t* picked_thread = NULL;
    uint32_t min_priority = 0xFFFFFFFF;

    for_each_thread(iter, thread_list) {
        thread_t* thread = entry_thread(iter);
        if (thread->cpu == -1 || thread->cpu == locals->core_id) {
            switch (thread->state) {
                case THREAD_STOPPED: {
                    if (thread->priority < min_priority) {
                        picked_thread = thread;
                        min_priority = thread->priority;
                    }
                } break;

                case THREAD_RUNNING:
                case THREAD_SLEEPING:
                case THREAD_TERMINATED:
                default:
                    break;
            }
        }
    }

    return picked_thread;
}

void save_thread(core_locals_t* locals, thread_t* thread, thread_state_t state) {
    assert(thread != NULL);

    thread->state = state;
    thread->regs = locals->irq_regs;
    // asm volatile("fxsave %0;" : : "m" (*thread->sse_region));
}

void load_thread(core_locals_t* locals, thread_t* thread) {
    assert(thread != NULL);

    // ser_printf("[%u][%s:%u] Picked\n", locals->core_id, thread->parent->name, thread->tid);

    if (locals->state == CORE_IDLE)
        locals->state = CORE_ONLINE;

    locals->current_thread = thread;
    locals->irq_regs = thread->regs;
    // asm volatile("fxrstor %0;" : : "m" (*thread->sse_region));

    thread->state = THREAD_RUNNING;
    set_kernel_stack(thread->kernel_stack);
}

void enter_idle(core_locals_t* locals) {
    assert(locals->current_thread == NULL);

    locals->state = CORE_IDLE;
    locals->current_thread = NULL;
    locals->irq_regs = locals->idle_thread->regs;
    set_kernel_stack(locals->idle_thread->kernel_stack);

    // ser_printf("[%u] Idle\n", locals->core_id);
}

void _sched(core_locals_t* locals) {
    if (locals->state == CORE_OFFLINE)
        return;

    // ser_printf("[%u] Thread list (c 0x%08x):\n", locals->core_id, thread);
    // if (have_thread()) {
    //     for_each_thread(iter, thread_list) {
    //         thread_t* thread = entry_thread(iter);

    //         ser_printf("[%u] 0x%08x: %s:%u:%u\n", locals->core_id, thread, thread->parent->name, thread->tid, thread->state);
    //     }
    // } else {
    //     ser_printf("[%u] Empty\n", locals->core_id);
    // }

    update_sleeping_threads(locals);

    thread_t* new_thread = pick_thread(locals);

    switch (locals->state) {
        case CORE_ONLINE: {
            assert(locals->current_thread != NULL);

            switch (locals->current_thread->state) {
                case THREAD_RUNNING: {
                    if (!new_thread)
                        return;

                    save_thread(locals, locals->current_thread, THREAD_STOPPED);
                    load_thread(locals, new_thread);
                } break;

                case THREAD_TERMINATED: {
                    ser_printf("[%u][%s:%u] Terminated with exitcode 0x%08x\n", locals->core_id, locals->current_thread->parent->name, locals->current_thread->tid, locals->current_thread->exitcode);

                    locals->current_thread = NULL;
                    if (!new_thread)
                        enter_idle(locals);
                    else
                        load_thread(locals, new_thread);
                } break;

                case THREAD_SLEEPING: {
                    save_thread(locals, locals->current_thread, THREAD_SLEEPING);
                    locals->current_thread = NULL;

                    if (!new_thread)
                        enter_idle(locals);
                    else
                        load_thread(locals, new_thread);
                } break;
            }
        } break;

        case CORE_IDLE:{
            assert(locals->current_thread == NULL);

            if (!new_thread)
                return;

            load_thread(locals, new_thread);
        } break;

        case CORE_BOOTING: {
            assert(locals->current_thread == NULL);

            enter_idle(locals);
        } break;

        default: {
            panic("Unknown core state!\n");
        }
    }
}

void _sched_runner(core_locals_t* locals) {
    check_and_lock(_sched_lock)
        _sched(locals);
        // remove_terminated_threads();
        // remove_terminated_processes();
        sched_unlock();
    }
}

void _panic(core_locals_t* locals) {
    ser_printf("Core %u in panic (indirect)!\n", locals->core_id);
    locals->state = CORE_OFFLINE;

    _idle();
}

void _sched_all(core_locals_t* locals) {
    if (!have_proc())
        return;

    if (!proc_have_thread(kernel_proc))
        return;

    _sched_runner(locals);
    smp_send_global_interrupt(0xF0);
}

void sched_run_thread(thread_t* thread) {
    interrupt_state_t state = interrupt_lock();
    sched_lock();

    assert(thread->state == THREAD_STOPPED);
    add_thread(thread);

    sched_unlock();
    interrupt_unlock(state);
}

void sched_run_proc(process_t* proc) {
    interrupt_state_t state = interrupt_lock();
    sched_lock();

    assert(!list_empty(PROC_THREAD_HEAD(proc)));

    add_proc(proc);

    struct list_head* iter;
    for_each_process_thread(iter, proc) {
        thread_t* thread = entry_process_thread(iter);
        assert(thread->state == THREAD_STOPPED);
        add_thread(thread);
    }

    sched_unlock();
    interrupt_unlock(state);
}

void sched_init_core() {
    core_locals_t* locals = get_core_locals();

    locals->idle_thread = thread_create(idle_proc, _idle);
    locals->current_thread = NULL;
    locals->state = CORE_BOOTING;
}

void sched_thread_exit(uint32_t code) {
    core_locals_t* locals = get_core_locals();

    if (locals->current_thread) {
        locals->current_thread->exitcode = code;
        locals->current_thread->state = THREAD_TERMINATED;
        _sched(locals);
    }
}

void sched_thread_sleep(uint32_t time_ms) {
    core_locals_t* locals = get_core_locals();

    if (locals->current_thread) {
        locals->current_thread->sleep_time = time_ms * 1000;
        locals->current_thread->state = THREAD_SLEEPING;
        _sched(locals);
    }
}

void sched_thread_yield() {
    core_locals_t* locals = get_core_locals();

    _sched_runner(locals);
}

void sched_init(void* entry) {
    interrupt_state_t state = interrupt_lock();

    ser_printf("[SCHED] Initializing scheduler...\n");
    INIT_LIST_HEAD(PROC_HEAD(&proc_list));
    INIT_LIST_HEAD(THREAD_HEAD(&thread_list));

    register_int_handler(0xF0, _sched_runner);
    register_int_handler(0xF1, _panic);

    syscalls_set(0, (void*)sched_thread_exit);
    syscalls_set(1, (void*)sched_thread_sleep);
    syscalls_set(2, (void*)sched_thread_yield);

    kernel_proc = proc_create_kernel("_kernel", 0);
    idle_proc   = proc_create_kernel("_idle", 0);

    thread_create(kernel_proc, entry);

    ser_printf("[SCHED] Scheduler initialized\n");

    interrupt_unlock(state);
}

void sched_run() {
    sched_run_proc(kernel_proc);
    syscall0(2);
}