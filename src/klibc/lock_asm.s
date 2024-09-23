[BITS 32]
[SECTION .text]

[GLOBAL spinlock_lock]
type spinlock_lock function
spinlock_lock:
    mov edi, [esp + 4]
    xor eax, eax
    lock bts dword [edi], 0
    jc  spin
    ret
spin:
    inc eax
    cmp eax, 0x10000000
    je  deadlock
    pause
    test dword [edi], 1
    jnz spin
    jmp spinlock_lock

[GLOBAL spinlock_unlock]
type spinlock_unlock function
spinlock_unlock:
    mov edi, [esp + 4]
    lock btr dword [edi], 0
    ret

[GLOBAL atomic_dec]
type atomic_dec function
atomic_inc:
    mov edi, [esp + 4]
    lock inc dword [edi]
    pushf
    pop eax
    not eax
    and eax, (1 << 6)
    shr eax, 6
    ret

[GLOBAL atomic_dec]
type atomic_dec function
atomic_dec:
    mov edi, [esp + 4]
    lock dec dword [edi]
    pushf
    pop eax
    not eax
    and eax, (1 <<  6)
    shr eax, 6
    ret

[EXTERN deadlock_handler]
type deadlock function
deadlock:
    push edi
    push edi

    xor eax, eax
    call deadlock_handler

    pop edi
    pop edi
    jmp spin

[GLOBAL spinlock_check_and_lock]
type spinlock_check_and_lock function
spinlock_check_and_lock:
    mov edi, [esp + 4]
    xor eax, eax
    lock bts dword [edi], 0
    setc al
    ret

[GLOBAL spinlock_with_timeout]
type spinlock_with_timeout function
spinlock_with_timeout:
    mov edi, [esp + 4]
    xor eax, eax
spin_timeout:
    inc eax

    lock bts dword [edi], 0

    setc bl
    cmp bl, 0
    je got_lock

    cmp eax, esi
    je timed_out

    pause
    jmp spin_timeout
got_lock:
    mov eax, 1
    ret
timed_out:
    xor eax, eax
    ret