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

    for (uint32_t i=0; i<sizeof(tss_t); i++) ((uint8_t*)&tss)[i] = 0;

    tss.ss0 = ss0;
    tss.esp0 = esp0;


    tss.iomap_base = sizeof(tss_t);

    uint32_t base  = (uint32_t)(uintptr_t)&tss;
    uint32_t limit = base + sizeof(tss_t);


    gdt_set_gate(num, base, limit, 0x89, 0x00);
}

void tss_set_kernel_stack(uint32_t esp0){
    tss.esp0 = esp0;
}

void gdt_init(void){
    gdtp.limit = (uint16_t)(sizeof(gdt_entry_t) * 6 - 1);
    gdtp.base  = (uint32_t)(uintptr_t)&gdt;


    gdt_set_gate(0, 0, 0, 0, 0);


    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);


    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);


    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);


    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);


    write_tss(5, 0x10, kstack_top);


    gdt_flush((uint32_t)(uintptr_t)&gdtp);


    tss_flush();
}
