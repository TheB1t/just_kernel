[BITS 32]
[SECTION .text]

[GLOBAL _idle]
type _idle function
_idle:
    hlt
    jmp _idle