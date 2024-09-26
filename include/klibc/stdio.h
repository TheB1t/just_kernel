#pragma once

#include <klibc/stdlib.h>
#include <klibc/stdarg.h>

typedef void (*printf_putchar_t)(char c, void* arg);

extern printf_putchar_t _global_putchar;

void sprintf(char* buf, char* format, ...);
void kprintf(char* format, ...);
void _vprintf(printf_putchar_t putchar, void* putchar_arg, char* format, va_list args);