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
extern uint64_t spinlock_check_and_lock(volatile uint32_t* lock);
extern uint64_t spinlock_with_timeout(volatile uint32_t* lock, uint64_t iterations);
extern uint32_t atomic_inc(volatile uint32_t* data);
extern uint32_t atomic_dec(volatile uint32_t* data);

#define lock(lock_) {                           \
    (lock_).attempting_to_get = __FUNCTION__;   \
    (lock_).name = #lock_;                      \
    spinlock_lock(&((lock_).dat));              \
    (lock_).holder = __FUNCTION__;              \
}                                               \

#define unlock(lock_) {                         \
    spinlock_unlock(&((lock_).dat));            \
}                                               \

interrupt_state_t   check_interrupts();
interrupt_state_t   interrupt_lock();
void                interrupt_unlock(interrupt_state_t state);