#include <drivers/pit.h>
#include <proc/sched.h>

volatile uint32_t global_ticks = 0;

void timer_handler(core_locals_t* locals) {
    global_ticks++;

    if (global_ticks % (8) == 0)
        _sched_all(locals);
}

void pit_set_freq() {
    uint16_t divisor = 1193;

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    port_outb(0x43, 0x36);
    port_outb(0x40, low);
    port_outb(0x40, high);
}

void pit_init() {
    register_int_handler(32, timer_handler);
    pit_set_freq();
}

void sleep_no_task(uint32_t ticks) {
    volatile uint32_t start_ticks = global_ticks;
    while (global_ticks < ticks + start_ticks)
        asm volatile("pause");
}

uint32_t stopwatch_start() {
    return global_ticks;
}

uint32_t stopwatch_stop(uint32_t start) {
    return (global_ticks - start);
}