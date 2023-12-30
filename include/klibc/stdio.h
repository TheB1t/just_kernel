#pragma once

#include <klibc/stdlib.h>
#include <klibc/stdarg.h>

typedef void (*printf_putchar_t)(char c);

extern printf_putchar_t _global_putchar;

void kprintf(const char* format, ...);
void _vprintf(printf_putchar_t putchar, const char* format, va_list args);