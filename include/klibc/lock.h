#pragma once

#include <klibc/stdlib.h>

typedef uint8_t interrupt_state_t;

typedef volatile struct {
    uint32_t dat;
    const char* name;
    const char* holder;
    const char* attempting_to_get;
} lock_t;

extern void spinlock_lock(volatile uint32_t* lock);
extern void spinlock_unlock(volatile uint32_t* lock);
extern uint32_t spinlock_check_and_lock(volatile uint32_t* lock);
extern uint32_t spinlock_with_timeout(volatile uint32_t* lock, uint32_t iterations);
extern uint32_t atomic_inc(volatile uint32_t* data);
extern uint32_t atomic_dec(volatile uint32_t* data);
extern uint8_t in_irq();

#define INIT_LOCK(lock_) { .dat = 0, .name = #lock_, .holder = "None", .attempting_to_get = "None" }

#define lock(lock_) do {                        \
    (lock_).attempting_to_get = __FUNCTION__;   \
    spinlock_lock(&((lock_).dat));              \
    (lock_).attempting_to_get = "None";         \
    (lock_).holder = __FUNCTION__;              \
} while (0)                                     \

#define _check_and_lock(lock_) spinlock_check_and_lock(&((lock_).dat))

#define check_and_lock(lock_)                   \
    if (!_check_and_lock(lock_)) {              \
        (lock_).holder = __FUNCTION__;          \

#define unlock(lock_) do {                      \
    spinlock_unlock(&((lock_).dat));            \
    (lock_).holder = "None";                    \
} while (0)                                     \

interrupt_state_t   check_interrupts();
interrupt_state_t   interrupt_lock();
void                interrupt_unlock(interrupt_state_t state);