#pragma once

#include <klibc/stdlib.h>
#include <int/isr.h>
#include <io/ports.h>

void        pit_init();
void        sleep_no_task(uint32_t ticks);

uint32_t    stopwatch_start();
uint32_t    stopwatch_stop(uint32_t start);