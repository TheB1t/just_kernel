#include <proc/sched.h>
#include <int/isr.h>
#include <sys/smp.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <klibc/lock.h>

#define add_thread(thread)      assert((uint32_t)thread != 0xFFFFFFF8); list_add_tail(THREAD_LIST(thread), THREAD_HEAD(&thread_list))
#define del_thread(thread)      assert((uint32_t)thread != 0xFFFFFFF8); list_del(THREAD_LIST(thread))

#define add_proc(proc)          assert((uint32_t)proc != 0xFFFFFFF8); list_add_tail(PROC_LIST(proc), PROC_HEAD(&proc_list))
#define del_proc(proc)          assert((uint32_t)proc != 0xFFFFFFF8); list_del(PROC_LIST(proc))

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
    sprintf("Core %u in panic (cause)!\n", get_core_locals()->core_index);

    smp_send_global_interrupt(0xF2);
}

static inline uint16_t get_fcw() {
    uint16_t buf;
    asm volatile("fnstcw (%0)" :: "r"(&buf) : "memory");
    return buf;
}

static inline void set_fcw(uint16_t buf) {
    asm volatile("fldcw (%0)" :: "r"(&buf) : "memory");
}

static inline uint32_t get_mxcsr() {
    uint32_t buf;
    asm volatile("stmxcsr (%0)" :: "r"(&buf) : "memory");
    return buf;
}

static inline void set_mxcsr(uint32_t buf) {
    asm volatile("ldmxcsr (%0)" :: "r"(&buf) : "memory");
}

void save_context(thread_t* thread, core_regs_t* regs) {
    thread->regs.eax    = regs->eax;
    thread->regs.ebx    = regs->ebx;
    thread->regs.ecx    = regs->ecx;
    thread->regs.edx    = regs->edx;
    thread->regs.ebp    = regs->ebp;
    thread->regs.edi    = regs->edi;
    thread->regs.esi    = regs->esi;

    thread->regs.eip    = regs->eip;
    thread->regs.cs     = regs->cs;
    thread->regs.eflags = regs->eflags;

    thread->regs.esp = regs->esp;
    thread->regs.ss  = regs->ss;

    if (thread->ring == 3) {
        thread->regs.esp = regs->esp0;
        thread->regs.ss  = regs->ss0;
    }

    // asm volatile("fxsave %0;"::"m"(thread->sse_region));

    thread->regs.mxcsr  = get_mxcsr();
    thread->regs.fcw    = get_fcw();
}

void load_context(thread_t* thread, core_regs_t* regs) {
    regs->eax    = thread->regs.eax;
    regs->ebx    = thread->regs.ebx;
    regs->ecx    = thread->regs.ecx;
    regs->edx    = thread->regs.edx;
    regs->ebp    = thread->regs.ebp;
    regs->edi    = thread->regs.edi;
    regs->esi    = thread->regs.esi;

    regs->eip    = thread->regs.eip;
    regs->cs     = thread->regs.cs;
    regs->eflags = thread->regs.eflags;
    regs->cr3    = thread->regs.cr3;

    regs->esp   = thread->regs.esp;
    regs->ss    = thread->regs.ss;

    if (thread->ring == 3) {
        regs->esp0 = thread->regs.esp;
        regs->ss0  = thread->regs.ss;
    }

    // asm volatile("fxrstor %0;"::"m"(thread->sse_region));
    // write_msr(0xC0000100, thread->regs.fs);

    set_mxcsr(thread->regs.mxcsr);
    set_fcw(thread->regs.fcw);
}

void _sched(core_locals_t* locals) {
    if (locals->state == CORE_OFFLINE)
        return;

    struct list_head* iter;
    thread_t* thread = locals->current_thread;
    thread_t* picked_thread = NULL;

    // isprintf("[%u] Thread list (c 0x%08x):\n", locals->core_index, thread);
    // if (have_thread()) {
    //     for_each_thread(iter, thread_list) {
    //         thread_t* thread = entry_thread(iter);

    //         isprintf("[%u] 0x%08x: %s:%u:%u\n", locals->core_index, thread, thread->parent->name, thread->tid, thread->state);
    //     }
    // } else {
    //     isprintf("[%u] Empty\n", locals->core_index);
    // }

    for_each_thread(iter, thread_list) {
        thread_t* t = entry_thread(iter);

        assert(t != NULL);

        if (t->state == THREAD_STOPPED && (t->cpu == -1 || t->cpu == locals->core_index)) {
            picked_thread = t;
            break;
        }
    }

    if (thread) {
        switch (thread->state) {
            case THREAD_RUNNING:
                if (!picked_thread)
                    return;

                save_context(thread, &locals->irq_regs);

                if (thread->regs.ebp == 0xDEADDEAD)
                    thread->state = THREAD_TERMINATED;
                else {
                    thread->state = THREAD_STOPPED;
                    // isprintf("[%u][%s:%u] Stopped\n", locals->core_index, thread->parent->name, thread->tid);
                    goto switch_thread;
                }

            case THREAD_TERMINATED:
                isprintf("[%u][%s:%u] Terminated\n", locals->core_index, thread->parent->name, thread->tid);
                locals->current_thread = NULL;
                break;

            switch_thread:
            default:
                add_thread(thread);
                locals->current_thread = NULL;
                break;
        }
    }

    if (picked_thread) {
        // isprintf("[%u][%s:%u] Picked\n", locals->core_index, picked_thread->parent->name, picked_thread->tid);

        thread = picked_thread;

        thread->state = THREAD_RUNNING;

        del_thread(thread);
        load_context(thread, &locals->irq_regs);

        locals->state = CORE_ONLINE;
        locals->current_thread = thread;

        // isprintf("[%u][%s:%u] Started\n", locals->core_index, thread->parent->name, thread->tid);
    } else if (!locals->current_thread && locals->state != CORE_IDLE) {
        assert(locals->idle_thread != NULL);

        locals->state = CORE_IDLE;
        load_context(locals->idle_thread, &locals->irq_regs);

        // isprintf("[%u] Idle\n", locals->core_index);
    }
}

void _sched_runner(core_locals_t* locals) {
    check_and_lock(_sched_lock)
        _sched(locals);
        unlock(_sched_lock);
    } else {
        // if (locals->current_thread)
        //     isprintf("Task %u on core %u overrunning\n", locals->current_thread->tid, locals->core_index);
        // else if (locals->state == CORE_IDLE)
        //     isprintf("Core %u overrunning in IDLE state (?)\n", locals->core_index);
    }
}

void _idle() {
    while (1) {
        asm volatile("hlt");
    }
}

void _panic(core_locals_t* locals) {
    isprintf("Core %u in panic (indirect)!\n", locals->core_index);
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
        add_thread(thread);
    }

    sched_unlock();
    interrupt_unlock(state);
}

void sched_init_core() {
    get_core_locals()->idle_thread = thread_create(idle_proc, _idle);
}

void sched_init(void* entry) {
    interrupt_state_t state = interrupt_lock();

    core_locals_t* locals = get_core_locals();

    sprintf("[SCHED] Initializing scheduler...\n");
    INIT_LIST_HEAD(PROC_HEAD(&proc_list));
    INIT_LIST_HEAD(THREAD_HEAD(&thread_list));

    register_int_handler(0xF0, _sched_runner);
    register_int_handler(0xF1, _sched_all);
    register_int_handler(0xF2, _panic);

    kernel_proc = proc_create("_kernel", entry, 0);
    idle_proc   = proc_create("_idle", _idle, 0);

    thread_t* thread    = get_main_thread(kernel_proc);
    thread->cpu         = -1;

    locals->idle_thread = get_main_thread(idle_proc);

    sprintf("[SCHED] Scheduler initialized\n");

    interrupt_unlock(state);
}

void sched_run() {
    interrupt_state_t state = interrupt_lock();

    sched_run_proc(kernel_proc);

    interrupt_unlock(state);
}