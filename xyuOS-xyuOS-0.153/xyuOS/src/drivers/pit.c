#include "pit.h"
#include "ports.h"

static volatile uint32_t g_ticks = 0;

uint32_t pit_ticks(void){
    return g_ticks;
}

void pit_on_tick(void){
    g_ticks++;
}

void pit_init(uint32_t hz){
    if(hz < 19) hz = 19;

    uint32_t divisor = 1193182 / hz;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}
