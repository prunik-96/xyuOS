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
#include "../editor/editor.h"



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
// console buffer for scrolling
#define CON_BUF_LINES 100
static char con_buf[CON_BUF_LINES][CON_W+1];
static int con_tail = 0; // next line index to write
static int con_count = 0; // number of filled lines (<= CON_BUF_LINES)
static int con_col = 0; // current column in tail line
static char input[128];
static int inlen = 0;

static void redraw_input(void){
    for(int i=0;i<34;i++) vga_print(IN_X+i, IN_Y, " ", 0x07);
    input[inlen] = 0;
    vga_print(IN_X, IN_Y, input, 0x0F);
    // draw cursor by inverting current cell at cursor position
    int cx = IN_X + inlen;
    int cy = IN_Y;
    uint16_t cur = vga_getcell(cx, cy);
    uint8_t attr = (cur >> 8) & 0xFF;
    uint8_t fg = attr & 0x0F;
    uint8_t bg = (attr >> 4) & 0x0F;
    uint8_t inv = ((fg << 4) & 0xF0) | (bg & 0x0F);
    uint16_t out = (cur & 0x00FF) | ((uint16_t)inv << 8);
    vga_putcell(cx, cy, out);
}



static void con_clear(void){
    for(int i=0;i<CON_BUF_LINES;i++) con_buf[i][0] = 0;
    con_tail = 0; con_count = 0; con_col = 0;
    for(int y=0;y<CON_H;y++)
        for(int x=0;x<CON_W;x++)
            vga_print(CON_X+x, CON_Y+y, " ", 0x07);
    conx = cony = 0;
}

static void con_putc(char c){
    if(c=='\n'){
        con_buf[con_tail][con_col] = 0;
        con_tail = (con_tail + 1) % CON_BUF_LINES;
        if(con_count < CON_BUF_LINES) con_count++; else { /* head drops */ }
        con_col = 0;
        conx = 0; cony = (con_count < CON_H) ? con_count-1 : CON_H-1;
        // redraw visible window
        int lines = (con_count < CON_H) ? con_count : CON_H;
        int start = (con_tail - lines + CON_BUF_LINES) % CON_BUF_LINES;
        for(int i=0;i<lines;i++){
            int idx = (start + i) % CON_BUF_LINES;
            vga_print(CON_X, CON_Y + i, con_buf[idx], 0x07);
            // clear remaining columns
            int len = kstrlen(con_buf[idx]);
            for(int x=len;x<CON_W;x++) vga_print(CON_X+x, CON_Y+i, " ", 0x07);
        }
        return;
    }
    if(con_col < CON_W-1){
        con_buf[con_tail][con_col++] = c;
        con_buf[con_tail][con_col] = 0;
    } else {
        // line full -> emit newline then put char
        con_buf[con_tail][con_col] = 0;
        con_tail = (con_tail + 1) % CON_BUF_LINES;
        if(con_count < CON_BUF_LINES) con_count++; else { }
        con_col = 0;
        con_buf[con_tail][con_col++] = c;
        con_buf[con_tail][con_col] = 0;
    }
    // draw tail line into visible area
    int lines = (con_count < CON_H) ? con_count : CON_H;
    int start = (con_tail - lines + CON_BUF_LINES) % CON_BUF_LINES;
    for(int i=0;i<lines;i++){
        int idx = (start + i) % CON_BUF_LINES;
        vga_print(CON_X, CON_Y + i, con_buf[idx], 0x07);
        int len = kstrlen(con_buf[idx]);
        for(int x=len;x<CON_W;x++) vga_print(CON_X+x, CON_Y+i, " ", 0x07);
    }
}

static void con_print(const char* s){
    for(size_t i=0;i<kstrlen(s);i++)
        con_putc(s[i]);
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
    con_print("[cmd] "); con_print(cmd); con_print("\n");

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
        file_t* f = fs_create(cmd+4);
        if(f){
            editor_set_area(CON_X, CON_Y, CON_W, CON_H);
            editor_open(f);
            editor_draw();
            con_print("[editor] opened\n");
            inlen = 0; input[0] = 0;
        } else {
            con_print("[error] cannot create file (fs full?)\n");
        }
        return;
    }

    if(kstartswith(cmd,"open ")){
        file_t* f = fs_get(cmd+5);
        if(f){ disk_load(f); editor_set_area(CON_X, CON_Y, CON_W, CON_H); editor_open(f); editor_draw(); con_print("[editor] opened\n"); inlen = 0; input[0] = 0; }
        return;
    }

    if(kstartswith(cmd,"write ")){
        if(editor_is_active()){
            // append to active editor
            editor_insert_string(cmd+6);
            editor_draw();
            return;
        }
        return;
    }

    if(kstrcmp(cmd,"save")==0){
        // legacy save command: editor handles saving (F2)
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

    // enable global blue theme (BSOD-like)
    vga_set_blue_theme(1);

    con_clear();
    con_print("xyuOS ready. type help\n");

    for(;;){
        keycode_t k;
        while(keyboard_pop(&k)){
            if(editor_is_active()){
                editor_handle_key(k);
                editor_draw();
                continue;
            }

            if(k==KEY_ENTER){
                exec_command(input);
                inlen = 0;
                input[0] = 0;
                redraw_input();
            }
            else if(k==KEY_BKSP && inlen){
                input[--inlen] = 0;
                redraw_input();
            }
            else if(k>=32 && k<=126 && inlen<127){
                input[inlen++] = (char)k;
                input[inlen] = 0;
                redraw_input();
            }
        }
        __asm__ volatile("hlt");
    }
}
