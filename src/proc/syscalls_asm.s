[BITS 32]
[SECTION .text]

%macro do_syscall 0
    int 0x80
    ret
%endmacro

[GLOBAL syscall0]
type syscall0 function
syscall0:
    mov eax, [esp + 4]
    do_syscall

[GLOBAL syscall1]
type syscall1 function
syscall1:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    do_syscall

[GLOBAL syscall2]
type syscall2 function
syscall2:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    do_syscall

[GLOBAL syscall3]
type syscall3 function
syscall3:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    do_syscall

[GLOBAL syscall4]
type syscall4 function
syscall4:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    mov esi, [esp + 20]
    do_syscall

[GLOBAL syscall5]
type syscall5 function
syscall5:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    mov ecx, [esp + 12]
    mov edx, [esp + 16]
    mov esi, [esp + 20]
    mov edi, [esp + 24]
    do_syscall