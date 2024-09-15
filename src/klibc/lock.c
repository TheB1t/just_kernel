#include <klibc/lock.h>

interrupt_state_t check_interrupts() {
    uint32_t eflags;
    asm volatile("pushf; pop %%eax; mov %%eax, %0;" : "=r"(eflags) :: "eax");
    uint8_t int_flag = (uint8_t)((eflags & (0x200)) >> 9);
    return (interrupt_state_t)int_flag;
}

interrupt_state_t interrupt_lock() {
    interrupt_state_t ret = check_interrupts();
    asm volatile("cli");
    return ret;
}

void interrupt_unlock(interrupt_state_t state) {
    if (state)
        asm volatile("sti");
}

void deadlock_handler(lock_t *lock) {
    ser_printf("Warning: Potential deadlock in lock %s held by %s\n", lock->name, lock->holder);
    ser_printf("Attempting to get lock from %s\n", lock->attempting_to_get);
}