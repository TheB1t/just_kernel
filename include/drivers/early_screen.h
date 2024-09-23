#pragma once

#include <klibc/stdlib.h>
#include <io/ports.h>
#include <drivers/vesa.h>

void            screen_putc(char c);
void            screen_puts(char* str);
void            screen_clear();
void            screen_init();