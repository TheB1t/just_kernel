[BITS 32]
[SECTION .text]

[GLOBAL ioapic_read]
type ioapic_read function
ioapic_read:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov [eax], ecx
    mov eax, [eax + 16]
    ret

[GLOBAL ioapic_write]
type ioapic_write function
ioapic_write:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    mov [eax], ecx
    mov ecx, [esp + 12]
    mov [eax + 16], ecx
    ret

[GLOBAL read_lapic]
type read_lapic function
read_lapic:
    mov eax, [esp + 4]
    and eax, 0xFFFF
    add eax, [lapic_base]
    mov eax, [eax]
    ret

[GLOBAL write_lapic]
type write_lapic function
write_lapic:
    mov eax, [esp + 4]
    mov ebx, [esp + 8]
    and eax, 0xFFFF
    add eax, [lapic_base]
    mov [eax], ebx
    ret

[SECTION .data]
[GLOBAL lapic_base]
lapic_base: dd 0xfee00000