BITS 32
GLOBAL gdt_flush
GLOBAL tss_flush

gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    ; обновляем сегменты на kernel data selector 0x10
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; far jump чтобы обновить CS на kernel code 0x08
    jmp 0x08:flush_done

flush_done:
    ret

tss_flush:
    mov ax, 0x28
    ltr ax
    ret
