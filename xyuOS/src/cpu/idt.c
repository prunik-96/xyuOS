#include "idt.h"

typedef struct {
  uint16_t base_lo;
  uint16_t sel;
  uint8_t  always0;
  uint8_t  flags;
  uint16_t base_hi;
} __attribute__((packed)) idt_entry_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idtp;

extern void idt_load(uint32_t);
extern void isr32(void);
extern void isr33(void);
extern void isr128(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags){
  idt[num].base_lo = base & 0xFFFF;
  idt[num].base_hi = (base >> 16) & 0xFFFF;
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
}

static inline void lidt(idt_ptr_t* p){
  __asm__ volatile("lidt (%0)"::"r"(p));
}

void idt_init(void){
  idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
  idtp.base  = (uint32_t)&idt;

  for(int i=0;i<256;i++) idt_set_gate((uint8_t)i, 0, 0, 0);

  // 0x08 — кодовый сегмент (GRUB обычно ставит плоский GDT; этого достаточно на старте)
  idt_set_gate(32,  (uint32_t)isr32,  0x08, 0x8E); // IRQ0
  idt_set_gate(33,  (uint32_t)isr33,  0x08, 0x8E); // IRQ1
  idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE); // int 0x80 (user-call, DPL=3)

  lidt(&idtp);
}
