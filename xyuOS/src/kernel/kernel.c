#include "../drivers/vga.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../util/string.h"
#include <stdint.h>

#define CON_X 42
#define CON_Y 4
#define CON_W 34
#define CON_H 10

#define IN_X  42
#define IN_Y  16

static int curx = 0, cury = 0;
static char input[128];
static int inlen = 0;

static void con_clear_area(void){
  for(int y=0; y<CON_H; y++){
    for(int x=0; x<CON_W; x++){
      vga_print(CON_X+x, CON_Y+y, " ", 0x07);
    }
  }
  curx = 0;
  cury = 0;
}

static void con_putc(char c){
  if(c=='\n'){
    curx = 0;
    cury++;
    if(cury >= CON_H) cury = CON_H - 1; // простая прокрутка позже
    return;
  }
  char s[2] = {c, 0};
  vga_print(CON_X + curx, CON_Y + cury, s, 0x07);
  curx++;
  if(curx >= CON_W){
    curx = 0;
    cury++;
    if(cury >= CON_H) cury = CON_H - 1;
  }
}

static void con_print(const char* s){
  for(size_t i=0; i<kstrlen(s); i++) con_putc(s[i]);
}

static void draw_ui(void){
  vga_clear(0x00);

  // рамка всего
  vga_box(0,0,80,25,0x07);

  // editor
  vga_box(1,1,38,17,0x0F);
  vga_print(3,2,"TEXT EDITOR",0x0C);

  // console
  vga_box(40,1,38,14,0x07);
  vga_print(42,2,"CONSOLE",0x0C);

  // input
  vga_box(40,15,38,3,0x07);
  vga_print(42,16,">",0x0F);

  // bottom
  vga_box(1,19,78,5,0x07);
  vga_print(3,20,"BOTTOM PANEL",0x0C);

  con_clear_area();
  con_print("help | clear | time | run <asm>\n");
  con_print("asm subset: nop, ret, hlt, int80, mov eax,imm, mov ebx,imm, db XX\n");
}

static void redraw_input(void){
  // очистить строку ввода
  for(int i=0; i<34; i++) vga_print(IN_X+i, IN_Y, " ", 0x07);

  vga_print(42,16,">",0x0F);
  input[inlen] = 0;
  vga_print(IN_X, IN_Y, input, 0x0F);
}

static int assemble_line(const char* line, uint8_t* out, int cap){
  while(*line==' '||*line=='\t') line++;
  if(!*line) return 0;

  if(kstartswith(line,"nop")){ if(cap<1) return -1; out[0]=0x90; return 1; }
  if(kstartswith(line,"ret")){ if(cap<1) return -1; out[0]=0xC3; return 1; }
  if(kstartswith(line,"hlt")){ if(cap<1) return -1; out[0]=0xF4; return 1; }
  if(kstartswith(line,"int80") || kstartswith(line,"int 0x80")){
    if(cap<2) return -1; out[0]=0xCD; out[1]=0x80; return 2;
  }

  if(kstartswith(line,"mov eax,")){
    if(cap<5) return -1;
    const char* p = line + 8;
    while(*p==' ') p++;

    uint32_t v=0;
    if(p[0]=='0' && (p[1]=='x'||p[1]=='X')){
      p+=2;
      while(1){
        int h=-1; char c=*p;
        if(c>='0'&&c<='9') h=c-'0';
        else if(c>='a'&&c<='f') h=10+(c-'a');
        else if(c>='A'&&c<='F') h=10+(c-'A');
        else break;
        v=(v<<4)|(uint32_t)h;
        p++;
      }
    } else {
      while(*p>='0'&&*p<='9'){ v = v*10 + (uint32_t)(*p-'0'); p++; }
    }

    out[0]=0xB8;
    out[1]=(uint8_t)(v&0xFF);
    out[2]=(uint8_t)((v>>8)&0xFF);
    out[3]=(uint8_t)((v>>16)&0xFF);
    out[4]=(uint8_t)((v>>24)&0xFF);
    return 5;
  }

  if(kstartswith(line,"mov ebx,")){
    if(cap<5) return -1;
    const char* p = line + 8;
    while(*p==' ') p++;

    uint32_t v=0;
    if(p[0]=='0' && (p[1]=='x'||p[1]=='X')){
      p+=2;
      while(1){
        int h=-1; char c=*p;
        if(c>='0'&&c<='9') h=c-'0';
        else if(c>='a'&&c<='f') h=10+(c-'a');
        else if(c>='A'&&c<='F') h=10+(c-'A');
        else break;
        v=(v<<4)|(uint32_t)h;
        p++;
      }
    } else {
      while(*p>='0'&&*p<='9'){ v = v*10 + (uint32_t)(*p-'0'); p++; }
    }

    out[0]=0xBB;
    out[1]=(uint8_t)(v&0xFF);
    out[2]=(uint8_t)((v>>8)&0xFF);
    out[3]=(uint8_t)((v>>16)&0xFF);
    out[4]=(uint8_t)((v>>24)&0xFF);
    return 5;
  }

  if(kstartswith(line,"db ")){
    const char* p=line+3;
    int n=0;
    while(*p){
      while(*p==' '||*p=='\t'||*p==',') p++;
      if(!p[0]||!p[1]) break;
      if(n>=cap) return -1;
      uint8_t b=0;
      if(!kparse_hex_byte(p,&b)) break;
      out[n++]=b;
      p+=2;
    }
    return n;
  }

  return -2;
}

static void run_asm(const char* asmtext){
  // небольшой буфер под машинный код
  static uint8_t code[256] __attribute__((aligned(16)));
  kmemset(code, 0x90, sizeof(code));
  int len=0;

  const char* p=asmtext;
  char line[64];
  int li=0;

  while(*p){
    char c=*p++;
    if(c=='\r') continue;
    if(c=='\n' || c==';'){
      line[li]=0; li=0;
      if(line[0]){
        int r=assemble_line(line, code+len, (int)sizeof(code)-len);
        if(r==-1){ con_print("ASM: too big\n"); return; }
        if(r==-2){ con_print("ASM: unknown instr\n"); return; }
        len += r;
      }
      continue;
    }
    if(li < (int)sizeof(line)-1) line[li++]=c;
  }

  line[li]=0;
  if(line[0]){
    int r=assemble_line(line, code+len, (int)sizeof(code)-len);
    if(r<0){ con_print("ASM: error\n"); return; }
    len += r;
  }

  // чтобы не улететь в мусор — гарантируем ret
  if(len==0 || code[len-1]!=0xC3){
    if(len < (int)sizeof(code)) code[len++]=0xC3;
  }

  con_print("RUN: executing...\n");

  // на всякий случай выключим IRQ на время выполнения (чтобы не ломали ввод),
  // а потом включим обратно
  __asm__ volatile("cli");

  void (*fn)(void) = (void(*)(void))(uintptr_t)code;
  fn();

  __asm__ volatile("sti");

  con_print("RUN: returned\n");
}

static void exec_command(const char* cmd){
  if(kstrcmp(cmd,"help")==0){
    con_print("help, clear, time, run <asm>\n");
    con_print("example: run nop; nop; ret\n");
    return;
  }
  if(kstrcmp(cmd,"clear")==0){
    con_clear_area();
    return;
  }
  if(kstrcmp(cmd,"time")==0){
    uint32_t t = pit_ticks();
    con_print("ticks=");
    char buf[16]; int i=0;
    uint32_t x=t;
    if(x==0){ buf[i++]='0'; }
    else{
      char tmp[16]; int j=0;
      while(x){ tmp[j++]=(char)('0'+(x%10)); x/=10; }
      while(j--) buf[i++]=tmp[j];
    }
    buf[i]=0;
    con_print(buf);
    con_putc('\n');
    return;
  }
  if(kstartswith(cmd,"run ")){
    run_asm(cmd+4);
    return;
  }
  con_print("unknown command\n");
}

void kernel_main(void){
  draw_ui();

  // ✅ ПРАВИЛЬНЫЙ ПОРЯДОК:
  // 1) GDT/TSS, чтобы сегменты 0x08/0x10 точно существовали
  gdt_init();

  // 2) IDT (обработчики IRQ/ISR)
  idt_init();

  // 3) PIC remap + unmask IRQ0/IRQ1
  pic_remap(0x20, 0x28);
  pic_clear_mask(0); // PIT
  pic_clear_mask(1); // Keyboard

  // 4) PIT + Keyboard
  pit_init(100);
  keyboard_init();

  // 5) включаем прерывания
  __asm__ volatile("sti");

  redraw_input();
  con_print("IRQ: enabled\n");

  for(;;){
    keycode_t k;
    while(keyboard_pop(&k)){
      if(k == KEY_ENTER){
        con_putc('\n');
        input[inlen]=0;
        exec_command(input);
        inlen=0;
        input[0]=0;
        redraw_input();
      } else if(k == KEY_BKSP){
        if(inlen>0){
          inlen--;
          input[inlen]=0;
          redraw_input();
        }
      } else if(k >= 32 && k <= 126){
        if(inlen < (int)sizeof(input)-1){
          input[inlen++] = (char)k;
          input[inlen]=0;
          redraw_input();
        }
      }
    }
    __asm__ volatile("hlt");
  }
}
