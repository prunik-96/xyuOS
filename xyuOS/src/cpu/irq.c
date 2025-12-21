#include "irq.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"

static void syscall_print(const char* s){
  // печатаем в нижнюю панель (простая: строка 23)
  vga_print(2, 23, s, 0x07);
}

void irq_handler(regs_t* r){
  if(r->int_no == 32){
    pit_on_tick();
    pic_send_eoi(0);
    return;
  }
  if(r->int_no == 33){
    keyboard_on_irq();
    pic_send_eoi(1);
    return;
  }
  if(r->int_no == 128){
    // eax = syscall id
    // 1: print (ebx = char*)
    if(r->eax == 1){
      syscall_print((const char*)r->ebx);
    }
    return;
  }
}
