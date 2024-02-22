MBOOT_PAGE_ALIGN    equ 1<<1
MBOOT_MEM_INFO      equ 1<<1
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]

[GLOBAL stack]
[GLOBAL mboot]
[EXTERN __kernel_code_start]
[EXTERN __kernel_bss_start]
[EXTERN __kernel_start]
[EXTERN __kernel_end]

mboot:
  dd  MBOOT_HEADER_MAGIC
  dd  MBOOT_HEADER_FLAGS
  dd  MBOOT_CHECKSUM
   
  dd  mboot
  dd  __kernel_code_start
  dd  __kernel_bss_start
  dd  __kernel_end
  dd  __kernel_start

section .bss

stack:
    resb 4096
  .top:

section .text

[GLOBAL start]
[EXTERN kmain]
type start function
start:
  xor ebp, ebp
  mov esp, stack

  push ebx
  ; Enable SSE
  mov eax, cr0
  and ax, 0xFFFB
  or  ax, 0x2
  mov cr0, eax
  mov eax, cr4
  or  ax, 3 << 9
  mov cr4, eax

  ; Execute the kernel
  cli
  call kmain  