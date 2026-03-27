#include "pic.h"
#include "ports.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2+1)

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

void pic_send_eoi(uint8_t irq){
  if(irq >= 8) outb(PIC2_COMMAND, 0x20);
  outb(PIC1_COMMAND, 0x20);
}

void pic_set_mask(uint8_t irqline){
  uint16_t port = (irqline < 8) ? PIC1_DATA : PIC2_DATA;
  if(irqline >= 8) irqline -= 8;
  uint8_t v = inb(port) | (1 << irqline);
  outb(port, v);
}

void pic_clear_mask(uint8_t irqline){
  uint16_t port = (irqline < 8) ? PIC1_DATA : PIC2_DATA;
  if(irqline >= 8) irqline -= 8;
  uint8_t v = inb(port) & ~(1 << irqline);
  outb(port, v);
}

void pic_remap(int offset1, int offset2){
  uint8_t a1 = inb(PIC1_DATA);
  uint8_t a2 = inb(PIC2_DATA);

  outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
  outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); io_wait();
  outb(PIC1_DATA, offset1); io_wait();
  outb(PIC2_DATA, offset2); io_wait();
  outb(PIC1_DATA, 4); io_wait();   // PIC2 at IRQ2
  outb(PIC2_DATA, 2); io_wait();
  outb(PIC1_DATA, ICW4_8086); io_wait();
  outb(PIC2_DATA, ICW4_8086); io_wait();

  outb(PIC1_DATA, a1);
  outb(PIC2_DATA, a2);
}
