BITS 32
GLOBAL isr32, isr33, isr128
EXTERN irq_handler

%macro ISR 1
isr%1:
    cli
    push dword 0
    push dword %1
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    sti
    iretd
%endmacro

ISR 32
ISR 33
ISR 128
