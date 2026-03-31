BITS 32

GLOBAL isr32
GLOBAL isr33
GLOBAL isr128

EXTERN irq_handler
EXTERN isr80_handler

; IRQ (hardware)

%macro IRQ 1
isr%1:
    cli
    push dword 0          ; error code
    push dword %1         ; interrupt number
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

IRQ 32
IRQ 33

; SYSCALL (int 0x80)

isr128:
    cli
    pusha

    call isr80_handler

    popa
    sti
    iretd
