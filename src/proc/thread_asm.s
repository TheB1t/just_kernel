[BITS 32]
[SECTION .text]

[GLOBAL load_thread]
type load_thread function
load_thread:
    call eax

    mov ebp, 0xDEADDEAD
.end:
    hlt
    jmp .end