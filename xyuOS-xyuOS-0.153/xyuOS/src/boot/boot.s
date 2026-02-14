BITS 32
GLOBAL _start
EXTERN kernel_main

SECTION .multiboot
align 4
dd 0x1BADB002
dd 0
dd -(0x1BADB002)

SECTION .text
_start:
    cli
    mov esp, stack_top
    call kernel_main

.halt:
    hlt
    jmp .halt

SECTION .bss
align 16
stack_bottom: resb 16384
stack_top:
