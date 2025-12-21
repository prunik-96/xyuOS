#include "keyboard.h"
#include "ports.h"

#define KBD_DATA 0x60

static volatile keycode_t q[128];
static volatile int qh=0, qt=0;

static int shift = 0;
static int tab_held = 0;

int keyboard_shift(void){ return shift; }
int keyboard_tab_held(void){ return tab_held; }

static void push(keycode_t k){
  int n = (qt+1) & 127;
  if(n==qh) return;
  q[qt]=k; qt=n;
}

int keyboard_pop(keycode_t* out){
  if(qh==qt) return 0;
  *out = q[qh];
  qh = (qh+1)&127;
  return 1;
}

static const char map_nomod[128]={
  0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
  9,'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
  'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
  'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
};

static const char map_shift[128]={
  0,27,'!','@','#','$','%','^','&','*','(',')','_','+',8,
  9,'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
  'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
  'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
};

void keyboard_init(void){
  // ничего
}

void keyboard_on_irq(void){
  uint8_t sc = inb(KBD_DATA);

  int release = (sc & 0x80) != 0;
  uint8_t code = sc & 0x7F;

  // shift
  if(code==42 || code==54){ shift = !release; return; }
  // tab held
  if(code==15){ tab_held = !release; if(!release) push(KEY_TAB); return; }

  // стрелки (set1 extended: E0 xx, мы упростим: если пришёл E0 — читаем следующий байт)
  if(sc==0xE0){
    uint8_t sc2 = inb(KBD_DATA);
    int rel2 = (sc2 & 0x80) != 0;
    uint8_t c2 = sc2 & 0x7F;
    if(rel2) return;
    if(c2==72) push(KEY_UP);
    else if(c2==80) push(KEY_DOWN);
    else if(c2==75) push(KEY_LEFT);
    else if(c2==77) push(KEY_RIGHT);
    return;
  }

  if(release) return;

  // enter/backspace
  if(code==28){ push(KEY_ENTER); return; }
  if(code==14){ push(KEY_BKSP); return; }

  char ch = shift ? map_shift[code] : map_nomod[code];
  if(ch) push((keycode_t)(uint8_t)ch);
}
