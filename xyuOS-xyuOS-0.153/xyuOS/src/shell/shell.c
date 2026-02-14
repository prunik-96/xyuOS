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



static void disk_save(file_t* f){
    if(!f) return;
    ata_write_sector(f->sector, (uint8_t*)f->data);
}

static void disk_load(file_t* f){
    if(!f) return;
    ata_read_sector(f->sector, (uint8_t*)f->data);
    f->size = kstrlen(f->data);
}



static void cmd_cpu(void){
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    __asm__ volatile("cpuid"
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

    if(kstrcmp(cmd,"help")==0){
        con_print("new open write save ls clear cpu sys\n");
        return;
    }

    if(kstrcmp(cmd,"clear")==0){
        con_clear();
        return;
    }

    if(kstrcmp(cmd,"cpu")==0){
        cmd_cpu();
        return;
    }

    if(kstrcmp(cmd,"sys")==0){
        con_print("xyuOS\n");
        con_print("FS: simple sector FS\n");
        con_print("Mode: kernel\n");
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
        if(cur_file->size + l >= MAX_FILE_SIZE) return;
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

    con_print("?\n");
}



void kernel_main(void){

    fs_init();

    gdt_init();
    idt_init();

    pic_remap(0x20,0x28);
    pic_clear_mask(0);
    pic_clear_mask(1);

    pit_init(100);
    keyboard_init();
    ata_init();

    __asm__ volatile("sti");

    con_clear();
    con_print("xyuOS ready. type help\n");

    for(;;){
        keycode_t k;
        while(keyboard_pop(&k)){
            if(k==KEY_ENTER){
                exec_command(input);
                inlen = 0;
                input[0] = 0;
            }
            else if(k==KEY_BKSP && inlen){
                input[--inlen] = 0;
            }
            else if(k>=32 && k<=126 && inlen<127){
                input[inlen++] = (char)k;
                input[inlen] = 0;
            }
        }
        __asm__ volatile("hlt");
    }
}
