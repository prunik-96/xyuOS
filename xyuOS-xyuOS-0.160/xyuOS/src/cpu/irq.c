#include "irq.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"



void irq_handler(regs_t* r){


    if(r->int_no == 32){
        pit_on_tick();
    }


    else if(r->int_no == 33){
        keyboard_on_irq();
    }


    if(r->int_no >= 40){
        pic_send_eoi(1);   // slave
    }


    pic_send_eoi(0);
}
