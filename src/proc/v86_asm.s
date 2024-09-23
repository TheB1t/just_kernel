[BITS 16]
[SECTION .text]

[GLOBAL v86_generic_int]

[GLOBAL v86_code_start]
[GLOBAL v86_code_end]
v86_code_start:

v86_generic_int:
    popa        ; Pop parameters
    int 0x00    ; Interrupt number will be replaced in v86 monitor
    jmp exit

exit:
    pusha       ; Push result
    int 0x03    ; Exit v86 mode...
inf_loop:
    jmp inf_loop    ; ...or suffer if something goes wrong

v86_code_end:

[BITS 32]
[SECTION .text]

[GLOBAL _enter_v86]
type _enter_v86 function
_enter_v86:
    cli

    mov ebp, esp

    mov eax, 0x0000
    push eax               ; GS
    push eax               ; FS
    push eax               ; DS
    push eax               ; ES

    mov eax, [ebp + 4]     ; v86 mode stack segment
    push eax               ; SS
    mov eax, [ebp + 8]     ; v86 mode stack pointer
    push eax               ; SP

    mov eax, 0x00020202    ; Set VM = 1, IOPL = 0, IF = 1
    push eax               ; Push EFLAGS

    mov eax, [ebp + 12]    ; v86 mode code segment
    push eax               ; CS
    mov eax, [ebp + 16]    ; v86 mode instruction pointer
    push eax               ; IP

    mov ebp, 0

    iret

[GLOBAL _exit_v86]
type _exit_v86 function
_exit_v86:
    push ebx    ; Pass saved registers as parameter
    call eax    ; Call handler