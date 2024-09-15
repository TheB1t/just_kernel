struc SMPBootInfo
	._status:   resb 1
    ._cr3:      resd 1
    ._esp:      resd 1
    ._entry:    resd 1
    ._gdt_ptr:  resd 1
endstruc

[BITS 16]
[SECTION .text]

[GLOBAL smp_trampoline]
[GLOBAL smp_trampoline_end]
smp_trampoline:
    cli
    mov edx, 0x500
    mov byte [edx + SMPBootInfo._status], 1

    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    mov eax, [edx + SMPBootInfo._gdt_ptr]
    lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

    jmp 0x08:(0x1000 + smp_trampoline_32 - smp_trampoline)

[BITS 32]
[SECTION .text]
smp_trampoline_32:
    mov eax, dword [edx + SMPBootInfo._cr3]
    mov cr3, eax

    mov eax, cr0
    or  eax, 0x80000000
    mov cr0, eax

    xor ebp, ebp
    mov esp, dword [edx + SMPBootInfo._esp]
    mov eax, dword [edx + SMPBootInfo._entry]
    jmp eax

smp_trampoline_end:

[EXTERN smp_boot_counter]
[EXTERN smp_entry_point]
[GLOBAL smp_loaded]
type smp_loaded function
smp_loaded:
    ; ; Init FPU
    ; fninit

    ; ; Enable SSE
    ; mov eax, cr0
    ; and ax, 0xFFFB
    ; or  ax, 0x2
    ; mov cr0, eax
    ; mov eax, cr4
    ; or  ax, 3 << 9
    ; mov cr4, eax

    mov byte [edx + SMPBootInfo._status], 2
    lock inc byte [smp_boot_counter]

    push edx

    cld
    xor eax, eax
    call smp_entry_point

.end:
    hlt
    jmp .end