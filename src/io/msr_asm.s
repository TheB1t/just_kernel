[BITS 32]
[SECTION .text]

[GLOBAL read_msr]
type read_msr function
read_msr:
    mov ecx, [esp + 4]
    rdmsr
    ret

[GLOBAL write_msr]
type write_msr function
write_msr:
    mov ecx, [esp + 4]
    mov eax, [esp + 8]
    mov edx, [esp + 12]
    wrmsr
    ret

[GLOBAL read_tsc]
type read_tsc function
read_tsc:
    rdtsc
    ret