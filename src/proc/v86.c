#include <proc/v86.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <int/isr.h>

v86_call_list_t     v86_call_list;
process_t*          v86_proc    = 0;
static uint8_t*     v86_ptr     = (uint8_t*)0x00007C00;
static uint8_t*     v86_stack   = (uint8_t*)0x0000A000;
static ivt_entry_t* ivt         = (ivt_entry_t*)0;

extern char v86_generic_int[];

extern char v86_code_start[];
extern char v86_code_end[];

extern void _enter_v86(uint32_t ss, uint32_t sp, uint32_t cs, uint32_t ip);
extern void _exit_v86();

#define _UINT32(var)                    ((uint32_t)(var))
#define _UINT16(var)                    ((uint16_t)(var))

#define FP_SEG(fp)                      (_UINT32(fp) >> 16)
#define FP_OFF(fp)                      (_UINT32(fp) & 0xffff)
#define FP_TO_LINEAR(seg, off)          ((void*)((_UINT16(seg) << 4) + _UINT16(off)))

#define SET_BIT(val, bit)               ((val) |= (1 << (bit)))
#define CLEAR_BIT(val, bit)             ((val) &= ~(1 << (bit)))
#define TEST_BIT(val, bit)              ((val) & (1 << (bit)))

#define _REL_SYM_ADDR(base, new, sym)   ((_UINT32(sym) - _UINT32(base)) + _UINT32(new))
#define REL_SYM_ADDR(sym)               _REL_SYM_ADDR(v86_code_start, v86_ptr, sym)

#ifdef V86_LOGGING
#define V86_LOG(fmt, ...)   ser_printf(fmt, ##__VA_ARGS__)
#else
#define V86_LOG(fmt, ...)   (void)0
#endif

void v86_call_generic_int(uint8_t int_num, v86_ctx_t* ctx) {
    ((uint8_t*)REL_SYM_ADDR(v86_generic_int))[2] = int_num;

    uint16_t stack = ctx->sp;

    stack -= sizeof(v86_ctx_t);
    uint8_t* stack_ptr = (uint8_t*)FP_TO_LINEAR(0x0000, stack);

    memcpy((uint8_t*)ctx, stack_ptr, sizeof(v86_ctx_t));

    _enter_v86(0x0000, stack, 0x0000, REL_SYM_ADDR(v86_generic_int));
}

void v86_enter() {
    while (1) {
        // TODO: add some yielding or event waiting
        while (v86_num_calls(&v86_call_list) == 0)
            asm volatile("pause");

        v86_call_t* call = v86_call_next(&v86_call_list);

        V86_LOG("running v86 call 0x%08x\n", call);

        call->ctx.sp = (uint16_t)(v86_stack);

        switch (call->type) {
            case V86_CALL_TYPE_GENERIC_INT: {
                v86_call_generic_int(call->int_num, &call->ctx);
            } break;

            default: {
                ser_printf("unhandled v86 call type: %d\n", call->type);
            } break;
        }
    }
}

void v86_exit(uint32_t stack) {
    v86_ctx_t* ctx = (v86_ctx_t*)FP_TO_LINEAR(0x0000, stack);

    V86_LOG("AX: 0x%04x, BX: 0x%04x, CX 0x%04x\n", ctx->ax, ctx->bx, ctx->cx);
    V86_LOG("DX: 0x%04x, SI: 0x%04x, DI 0x%04x\n", ctx->dx, ctx->si, ctx->di);
    V86_LOG("BP: 0x%04x, SP: 0x%04x\n", ctx->bp, ctx->sp);

    v86_call_t* call = v86_call_next(&v86_call_list);
    del_v86_call(call);

    memcpy((uint8_t*)ctx, (uint8_t*)&call->ctx, sizeof(v86_ctx_t));
    call->type = V86_CALL_TYPE_COMPLETE;

    V86_LOG("done with v86 call 0x%08x\n", call);

    v86_enter();
}

void v86_push(core_regs_t* regs, uint32_t value, bool is32) {
    uint16_t stack = regs->user.esp;

    if (is32) {
        stack -= 4;
        *(uint32_t*)FP_TO_LINEAR(regs->user.ss, stack) = value;
    } else {
        stack -= 2;
        *(uint16_t*)FP_TO_LINEAR(regs->user.ss, stack) = value;
    }

    regs->user.esp = stack;
}

uint32_t v86_pop(core_regs_t* regs, bool is32) {
    uint16_t stack = regs->user.esp;
    uint32_t value = 0;

    if (is32) {
        value = *(uint32_t*)FP_TO_LINEAR(regs->user.ss, stack);
        stack += 4;
    } else {
        value = *(uint16_t*)FP_TO_LINEAR(regs->user.ss, stack);
        stack += 2;
    }

    regs->user.esp = stack;
    return value;
}

uint8_t v86_get_ip_byte(core_regs_t* regs) {
    uint8_t* ip_ptr = (uint8_t*)FP_TO_LINEAR(regs->base.cs, regs->base.eip);

    (*(uint16_t*)&regs->base.eip)++;
    return ip_ptr[0];
}

void v86_monitor(core_locals_t* locals) {
    core_regs_t* regs = locals->irq_regs;
    thread_t* thread = locals->current_thread;

    bool is_operand32 = false;
    bool is_address32 = false;

    V86_LOG("v86_monitor: cs:ip = %04x:%04x ss:sp = %04x:%04x: ", regs->base.cs, regs->base.eip, regs->user.ss, regs->user.esp);

    while (true) {
        uint8_t instr = v86_get_ip_byte(regs);

        switch (instr) {
            case 0x66: {  /* O32 */
                V86_LOG("o32 ");
                is_operand32 = true;
            } break;

            case 0x67: {  /* A32 */
                V86_LOG("a32 ");
                is_address32 = true;
            } break;

            case 0x9c: { /* PUSHF */
                V86_LOG("pushf\n");

                uint32_t eflags = regs->base.eflags | 0x20002;

                if (thread->flags.v86_if)
                    SET_BIT(eflags, 9);
                else
                    CLEAR_BIT(eflags, 9);

                v86_push(regs, eflags, is_operand32);
            } goto _ok;

            case 0x9d: { /* POPF */
                V86_LOG("popf\n");

                uint32_t eflags = v86_pop(regs, is_operand32) | 0x20002;
                thread->flags.v86_if = TEST_BIT(eflags, 9) != 0;
                regs->base.eflags = eflags;
            } goto _ok;

            case 0xcd: { /* INT n */
                uint8_t int_num = v86_get_ip_byte(regs);
                V86_LOG("interrupt 0x%x => ", int_num);

                switch (int_num) {
                    case 0x03: {
                        V86_LOG("v86_exit\n");
                        regs->base.cs = DESC_SEG(DESC_KERNEL_CODE, PL_RING0);
                        regs->base.ds = DESC_SEG(DESC_KERNEL_DATA, PL_RING0);
                        regs->base.eip = (uint32_t)_exit_v86;
                        regs->base.eflags = 0x202;
                        regs->base.esp = (uint32_t)thread->kernel_stack;
                        regs->base.eax = (uint32_t)v86_exit;
                        regs->base.ebx = regs->user.esp;
                    } break;

                    default: {
                        uint32_t eflags = regs->base.eflags | 0x20002;

                        if (thread->flags.v86_if)
                            SET_BIT(eflags, 9);
                        else
                            CLEAR_BIT(eflags, 9);

                        v86_push(regs, eflags, false);
                        v86_push(regs, regs->base.cs, false);
                        v86_push(regs, regs->base.eip, true);

                        regs->base.cs    = ivt[int_num].seg;
                        regs->base.eip   = ivt[int_num].off;
                        V86_LOG("%04x:%04x\n", regs->base.cs, regs->base.eip);
                    } break;
                }
            } goto _ok;

            case 0xcf: { /* IRET */
                regs->base.eip      = v86_pop(regs, true);
                regs->base.cs       = v86_pop(regs, false);
                uint32_t eflags     = v86_pop(regs, false) | 0x20002;
                regs->base.eflags   = eflags;
                thread->flags.v86_if = TEST_BIT(eflags, 9) != 0;

                V86_LOG("iret => %04x:%04x\n", regs->base.cs, regs->base.eip);
            } goto _ok;

            case 0xfa: { /* CLI */
                V86_LOG("cli\n");
                thread->flags.v86_if = false;
            } goto _ok;

            case 0xfb: { /* STI */
                V86_LOG("sti\n");
                thread->flags.v86_if = true;
            } goto _ok;

            default: {
                V86_LOG("unhandled opcode 0x%x\n", instr);
            } goto _err;
        }
    }

_ok:
    return;

_err:
    panic("v86_monitor: fatal error");
}

void v86_init() {
    INIT_LIST_HEAD(LIST_GET_HEAD(&v86_call_list));

    vmm_map((void*)0, (void*)0, 0x100000 / PAGE_SIZE, VMM_PRESENT | VMM_WRITE | VMM_USER);

    memcpy((void*)v86_code_start, v86_ptr, v86_code_end - v86_code_start);
    register_int_handler(0x0D, v86_monitor);

    v86_proc = proc_create_kernel("_v86", 0);
    thread_create(v86_proc, v86_enter);
    sched_run_proc(v86_proc);
}

void v86_call(v86_call_t* call) {
    if (!v86_proc)
        return;

    INIT_LIST_HEAD(LIST_GET_LIST(call));

    add_v86_call(&v86_call_list, call);

    // TODO: add some yielding or event waiting
    while (call->type != V86_CALL_TYPE_COMPLETE)
        asm volatile("pause");
}

void* v86_farptr2linear(uint32_t fp) {
    return FP_TO_LINEAR(FP_SEG(fp), FP_OFF(fp));
}