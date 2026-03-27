#include "editor.h"
#include "../drivers/vga.h"
#include "../util/string.h"

#define ED_X 2
#define ED_Y 4
#define ED_W 34
#define ED_H 12

static file_t* current_file = 0;

void editor_init(void){
    current_file = 0;
}

void editor_open(file_t* f){
    current_file = f;
}

void editor_draw(void){

    for(int y=0;y<ED_H;y++){
        for(int x=0;x<ED_W;x++){
            vga_print(ED_X+x, ED_Y+y, " ", 0x07);
        }
    }

    if(!current_file) return;

    int x=0, y=0;
    for(int i=0;i<current_file->size;i++){
        char c = current_file->data[i];
        if(c=='\n'){
            x=0;
            if(++y >= ED_H) break;
            continue;
        }

        char s[2] = {c, 0};
        vga_print(ED_X+x, ED_Y+y, s, 0x0F);

        if(++x >= ED_W){
            x=0;
            if(++y >= ED_H) break;
        }
    }
}

void editor_save(void){

}
