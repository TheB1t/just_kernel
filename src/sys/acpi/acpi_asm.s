[GLOBAL ll_find_rsdp]
type ll_find_rsdp function
ll_find_rsdp:
    mov eax, 0x0E0000
    mov edx, 0x0FFFFF

check_loop:
    cmp eax, edx
    jg not_supported

    mov ebx, [eax]
    cmp ebx, 'RSD '
    jne skip2

    mov ebx, [eax + 4]
    cmp ebx, 'PTR '
    je supported

skip2:
    add eax, 4
    jmp check_loop

not_supported:
    mov eax, 0
    ret
    
supported:
    ret