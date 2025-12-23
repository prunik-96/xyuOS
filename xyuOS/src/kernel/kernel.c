extern void isr32();
extern void isr33();
extern void isr128();




#include "../drivers/vga.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/ata.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../util/string.h"
#include "../fs/fs.h"
#include <stdint.h>




#define ED_X 2
#define ED_Y 4
#define ED_W 34
#define ED_H 12

#define CON_X 42
#define CON_Y 4
#define CON_W 34
#define CON_H 10

#define IN_X  42
#define IN_Y  16

static int conx = 0, cony = 0;
static char input[128];
static int inlen = 0;


static file_t* cur_file = 0;



static void con_clear(void){
    for(int y=0;y<CON_H;y++)
        for(int x=0;x<CON_W;x++)
            vga_print(CON_X+x, CON_Y+y, " ", 0x07);
    conx = cony = 0;
}

static void con_putc(char c){
    if(c=='\n'){
        conx = 0;
        if(++cony >= CON_H) cony = CON_H - 1;
        return;
    }
    char s[2] = {c,0};
    vga_print(CON_X+conx, CON_Y+cony, s, 0x07);
    if(++conx >= CON_W){
        conx = 0;
        if(++cony >= CON_H) cony = CON_H - 1;
    }
}

static void con_print(const char* s){
    for(size_t i=0;i<kstrlen(s);i++)
        con_putc(s[i]);
}



static void editor_draw(void){
    for(int y=0;y<ED_H;y++)
        for(int x=0;x<ED_W;x++)
            vga_print(ED_X+x, ED_Y+y, " ", 0x07);

    if(!cur_file) return;

    int x=0,y=0;
    for(int i=0;i<cur_file->size;i++){
        char c = cur_file->data[i];
        if(c=='\n'){
            x=0; y++;
            if(y>=ED_H) break;
            continue;
        }
        char s[2]={c,0};
        vga_print(ED_X+x, ED_Y+y, s, 0x0F);
        if(++x>=ED_W){
            x=0; y++;
            if(y>=ED_H) break;
        }
    }
}



static void draw_ui(void){
    vga_clear(0x00);
    vga_box(0,0,80,25,0x07);

    vga_box(1,1,38,17,0x0F);
    vga_print(3,2,"TEXT EDITOR  F1=open  F2=save",0x0C);

    vga_box(40,1,38,14,0x07);
    vga_print(42,2,"CONSOLE",0x0C);

    vga_box(40,15,38,3,0x07);
    vga_print(42,16,">",0x0F);

    vga_box(1,19,78,5,0x07);
    vga_print(3,20,"BOTTOM",0x0C);

    con_clear();
    con_print("WELCOME TO XUYOS 0.153 |write help if you need help|\n");
}

static void redraw_input(void){
    for(int i=0;i<34;i++)
        vga_print(IN_X+i, IN_Y, " ", 0x07);
    input[inlen] = 0;
    vga_print(IN_X, IN_Y, input, 0x0F);
}



static void disk_save(file_t* f){
    if(!f) return;
    __asm__ volatile("cli");
    ata_write_sector(f->sector, (uint8_t*)f->data);
    __asm__ volatile("sti");
}

static void disk_load(file_t* f){
    if(!f) return;
    __asm__ volatile("cli");
    ata_read_sector(f->sector, (uint8_t*)f->data);
    __asm__ volatile("sti");
    f->data[MAX_FILE_SIZE - 1] = 0;
    f->size = kstrlen(f->data);
}

static void cmd_cpu(void){
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    __asm__ volatile(
        "cpuid"
        : "=b"(ebx), "=d"(edx), "=c"(ecx)
        : "a"(0)
    );

    *(uint32_t*)&vendor[0] = ebx;
    *(uint32_t*)&vendor[4] = edx;
    *(uint32_t*)&vendor[8] = ecx;
    vendor[12] = 0;

    con_print("CPU: ");
    con_print(vendor);
    con_putc('\n');
}





static void exec_command(const char* cmd){





    if(kstrcmp(cmd,"uptime")==0){
        uint32_t t = pit_ticks();
        con_print("ticks: ");

        char buf[16];
        int i = 0;
        if(t == 0){
            buf[i++] = '0';
        } else {
            char tmp[16];
            int j = 0;
            while(t){
                tmp[j++] = '0' + (t % 10);
                t /= 10;
            }
            while(j--) buf[i++] = tmp[j];
        }
        buf[i] = 0;

        con_print(buf);
        con_putc('\n');
        return;
    }


    if(kstrcmp(cmd,"cpu")==0){
        cmd_cpu();
        return;
    }

    if(kstartswith(cmd,"rm ")){
        file_t* f = fs_get(cmd+3);
        if(!f){
            con_print("file not found\n");
            return;
        }
        f->used = 0;
        con_print("removed\n");
        return;
    }



    if(kstartswith(cmd,"cat ")){
        file_t* f = fs_get(cmd+4);
        if(!f){
            con_print("file not found\n");
            return;
        }
        con_print(f->data);
        con_putc('\n');
        return;
    }


    if(kstrcmp(cmd,"clear")==0){
        con_clear();
        return;
    }


    if(kstrcmp(cmd,"help")==0){
        con_print("|- ls, new, open, write, save, clear, cat, rm, cpu, uptime, sys. -|\n");
        return;
    }

    if(kstrcmp(cmd,"ls")==0){
        int n = fs_count();
        for(int i=0;i<n;i++){
            file_t* f = fs_at(i);
            con_print(" - ");
            con_print(f->name);
            con_putc('\n');
        }
        return;
    }

    if(kstartswith(cmd,"new ")){
        cur_file = fs_create(cmd+4);
        editor_draw();
        return;
    }

    if(kstartswith(cmd,"open ")){
        cur_file = fs_get(cmd+5);
        if(cur_file) disk_load(cur_file);
        editor_draw();
        return;
    }

    if(kstartswith(cmd,"write ")){
        if(!cur_file) return;
        int l = kstrlen(cmd+6);
        kmemcpy(cur_file->data + cur_file->size, cmd+6, l);
        cur_file->size += l;
        cur_file->data[cur_file->size++] = '\n';
        cur_file->data[cur_file->size] = 0;
        editor_draw();
        return;
    }

    if(kstrcmp(cmd,"save")==0){
        if(cur_file) disk_save(cur_file);
        return;
    }



    if(kstrcmp(cmd,"sys")==0){
      con_print("xyuOS\n");
      con_print("Mode: kernel\n");
      con_print("FS: simple sector fs\n");
      con_print("IRQ: enabled\n");
      return;
    }

    con_print("?\n");
}



void isr80_handler(void){

    vga_print(2, 22, "SYSCALL 0x80", 0x0A);
}

void kernel_main(void){

    fs_init();
    draw_ui();

    gdt_init();
    idt_init();

    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    pic_remap(0x20,0x28);
    pic_set_mask(14);
    pic_clear_mask(0);
    pic_clear_mask(1);

    pit_init(100);
    keyboard_init();
    ata_init();

    __asm__ volatile("sti");

    editor_draw();

    for(;;){
        keycode_t k;
        while(keyboard_pop(&k)){

            if(k==KEY_F1 && cur_file){
                disk_load(cur_file);
                editor_draw();
            }

            else if(k==KEY_F2 && cur_file){
                disk_save(cur_file);
            }

            else if(k==KEY_ENTER){
                exec_command(input);
                inlen = 0;
                redraw_input();
            }

            else if(k==KEY_BKSP && inlen){
                input[--inlen] = 0;
                redraw_input();
            }

            else if(k>=32 && k<=126 && inlen < 127){
                input[inlen++] = (char)k;
                redraw_input();
            }
        }
        __asm__ volatile("hlt");
    }
}

