#pragma once

#include <klibc/stdlib.h>
#include <int/isr.h>
#include <io/ports.h>

#define PIT_BASE_SPEED  1193182
#define PIT_SPEED_HZ    100
#define PIT_DIVISOR     (uint32_t)(PIT_BASE_SPEED / PIT_SPEED_HZ)
#define PIT_SPEED_US    (uint32_t)(double)(((double)PIT_DIVISOR / PIT_BASE_SPEED) * 1000000)

void        pit_init();
void        sleep_no_task(uint32_t ticks);

uint32_t    stopwatch_start();
uint32_t    stopwatch_stop(uint32_t start);