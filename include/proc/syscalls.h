#pragma once

#include <klibc/stdlib.h>

void syscalls_init();
void syscalls_set(uint32_t num, void* call);

extern void syscall0(uint32_t num);
extern void syscall1(uint32_t num, uint32_t arg1);
extern void syscall2(uint32_t num, uint32_t arg1, uint32_t arg2);
extern void syscall3(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3);
extern void syscall4(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
extern void syscall5(uint32_t num, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);