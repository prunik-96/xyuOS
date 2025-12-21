#include "gdt.h"
#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

// x86 TSS (32-bit)
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

static gdt_entry_t gdt[6];
static gdt_ptr_t   gdtp;
static tss_t       tss;

// отдельный kernel stack для будущего ring3->ring0 (syscall/irq)
static uint8_t kstack[16384];
static uint32_t kstack_top = (uint32_t)(uintptr_t)&kstack[sizeof(kstack)];

extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran){
    gdt[num].base_low  = (base & 0xFFFF);
    gdt[num].base_mid  = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].gran      = (limit >> 16) & 0x0F;

    gdt[num].gran     |= (gran & 0xF0);
    gdt[num].access    = access;
}

static void write_tss(int num, uint16_t ss0, uint32_t esp0){
    // очистим TSS
    for (uint32_t i=0; i<sizeof(tss_t); i++) ((uint8_t*)&tss)[i] = 0;

    tss.ss0 = ss0;
    tss.esp0 = esp0;

    // I/O map: отключаем доступ к портам из ring3 (по умолчанию)
    tss.iomap_base = sizeof(tss_t);

    uint32_t base  = (uint32_t)(uintptr_t)&tss;
    uint32_t limit = base + sizeof(tss_t);

    // Access: 0x89 = Present | DPL0 | Type=0x9 (Available 32-bit TSS)
    // Gran:   0x00 (byte granularity for TSS)
    gdt_set_gate(num, base, limit, 0x89, 0x00);
}

void tss_set_kernel_stack(uint32_t esp0){
    tss.esp0 = esp0;
}

void gdt_init(void){
    gdtp.limit = (uint16_t)(sizeof(gdt_entry_t) * 6 - 1);
    gdtp.base  = (uint32_t)(uintptr_t)&gdt;

    // 0: null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: kernel code 0x08
    // access: 0x9A = Present | ring0 | code | readable
    // gran:   0xCF = 4K gran + 32-bit + limit high
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 2: kernel data 0x10
    // access: 0x92 = Present | ring0 | data | writable
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 3: user code 0x18 (selector будет 0x1B с RPL=3)
    // access: 0xFA = Present | ring3 | code | readable
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4: user data 0x20 (selector будет 0x23 с RPL=3)
    // access: 0xF2 = Present | ring3 | data | writable
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5: TSS 0x28
    write_tss(5, 0x10, kstack_top);

    // загрузить GDT + обновить сегменты
    gdt_flush((uint32_t)(uintptr_t)&gdtp);

    // загрузить TR (TSS selector = 0x28)
    tss_flush();
}
