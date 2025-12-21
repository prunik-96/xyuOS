#pragma once
#include <stdint.h>

typedef enum {
  KEY_NONE=0, KEY_ENTER=13, KEY_BKSP=8, KEY_TAB=9,
  KEY_UP=1001, KEY_DOWN, KEY_LEFT, KEY_RIGHT
} keycode_t;

void keyboard_init(void);
void keyboard_on_irq(void);
int  keyboard_pop(keycode_t* out);
int  keyboard_shift(void);
int  keyboard_tab_held(void);
