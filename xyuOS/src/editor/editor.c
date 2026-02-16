#include "editor.h"
#include "../drivers/vga.h"
#include "../util/string.h"
#include "../drivers/ata.h"

static file_t* current_file = 0;
static int ed_x = 2, ed_y = 4, ed_w = 34, ed_h = 12;
static int caret_pos = 0;
static int top_line = 0;
static int visual_line_of(int p);
// compute visual column+line for position (before character at index p)
static void visual_pos_of(int p, int* out_line, int* out_col){
    if(!current_file){ *out_line = 0; *out_col = 0; return; }
    int x = 0;
    int line = 0;
    for(int i=0;i<p && i<current_file->size;i++){
        char c = current_file->data[i];
        if(c == '\n'){
            x = 0; line++; continue;
        }
        x++;
        if(x >= ed_w){ x = 0; line++; }
    }
    *out_line = line;
    *out_col = x;
}

// keep caret visible with a small margin (lines from top/bottom)
#define CARET_MARGIN 1

static void ensure_caret_visible(void){
    int ln = visual_line_of(caret_pos);
    if(ln < top_line + CARET_MARGIN) {
        top_line = ln - CARET_MARGIN;
        if(top_line < 0) top_line = 0;
    }
    if(ln >= top_line + ed_h - CARET_MARGIN){
        top_line = ln - ed_h + 1 + CARET_MARGIN;
        if(top_line < 0) top_line = 0;
    }
}

void editor_init(void){
    current_file = 0;
    caret_pos = 0;
}

void editor_set_area(int x, int y, int w, int h){
    ed_x = x; ed_y = y; ed_w = w; ed_h = h;
}

void editor_open(file_t* f){
    current_file = f;
    caret_pos = (f && f->size>0) ? f->size : 0;
}

int editor_is_active(void){
    return current_file != 0;
}

static void draw_controls(void){
    int bx = ed_x;
    int by = ed_y + ed_h;
    // clear control line
    for(int i=0;i<ed_w;i++) vga_print(bx+i, by, " ", 0x07);
    vga_print(bx+1, by, "[F2 Save]", 0x1F);
    vga_print(bx+12, by, "[F1 Exit]", 0x1F);
}

static void redraw_editor_contents(void){
    for(int y=0;y<ed_h;y++){
        for(int x=0;x<ed_w;x++){
            vga_print(ed_x+x, ed_y+y, " ", 0x07);
        }
    }

    if(!current_file) return;

    int x=0, y=0, line=0;
    for(int i=0;i<current_file->size;i++){
        char c = current_file->data[i];
        if(c=='\n'){
            x=0; line++; if(line >= top_line + ed_h) break; if(line >= top_line) y++; continue;
        }
        if(line >= top_line){
            // draw visible char
            char s[2]={c,0};
            if(y < ed_h && x < ed_w) vga_print(ed_x+x, ed_y+y, s, 0x0F);
        }
        if(++x>=ed_w){ x=0; line++; if(line >= top_line + ed_h) break; if(line >= top_line) y++; }
    }
}

void editor_draw(void){
    redraw_editor_contents();
    draw_controls();

    if(!current_file) return;

    // draw caret as inverse color at caret_pos
    int vline = 0, vcol = 0;
    visual_pos_of(caret_pos, &vline, &vcol);
    // translate to visible coordinates
    if(vline >= top_line && vline < top_line + ed_h){
        int screen_y = vline - top_line;
        int screen_x = vcol;
        if(screen_x < 0) screen_x = 0;
        if(screen_x >= ed_w) screen_x = ed_w - 1;
        int sx = ed_x + screen_x;
        int sy = ed_y + screen_y;
        uint16_t cur = vga_getcell(sx, sy);
        uint8_t attr = (cur >> 8) & 0xFF;
        uint8_t fg = attr & 0x0F;
        uint8_t bg = (attr >> 4) & 0x0F;
        uint8_t inv = ((fg << 4) & 0xF0) | (bg & 0x0F);
        uint16_t out = (cur & 0x00FF) | ((uint16_t)inv << 8);
        vga_putcell(sx, sy, out);
    }
}

static void insert_char_at(int pos, char c){
    if(!current_file) return;
    if(current_file->size + 1 >= MAX_FILE_SIZE) return;
    for(int i=current_file->size; i>pos; i--) current_file->data[i] = current_file->data[i-1];
    current_file->data[pos] = c;
    current_file->size++;
    current_file->data[current_file->size] = 0;
}

static void delete_char_before(int pos){
    if(!current_file) return;
    if(pos<=0) return;
    for(int i=pos-1;i<current_file->size-1;i++) current_file->data[i] = current_file->data[i+1];
    current_file->size--;
    current_file->data[current_file->size] = 0;
}

static int line_start_of(int p){
    if(!current_file) return 0;
    int i = p-1;
    while(i>=0 && current_file->data[i] != '\n') i--;
    return i+1;
}

// compute visual line index for a file position (counts wrapped lines)
static int visual_line_of(int p){
    if(!current_file) return 0;
    int x = 0;
    int line = 0;
    for(int i=0;i<p && i<current_file->size;i++){
        char c = current_file->data[i];
        if(c == '\n'){
            x = 0; line++; continue;
        }
        x++;
        if(x >= ed_w){ x = 0; line++; }
    }
    return line;
}

static int line_length_from(int start){
    if(!current_file) return 0;
    int i = start; int l = 0;
    while(i < current_file->size && current_file->data[i] != '\n'){ l++; i++; }
    return l;
}

void editor_handle_key(keycode_t k){
    if(!current_file) return;

    if(k==KEY_F2){
        editor_save();
        return;
    }

    if(k==KEY_F1){
        // close editor
        current_file = 0;
        return;
    }

    if(k==KEY_BKSP){
        if(caret_pos>0){ delete_char_before(caret_pos); caret_pos--; }
        ensure_caret_visible();
        return;
    }

    if(k==KEY_LEFT){ if(caret_pos>0) caret_pos--; ensure_caret_visible(); return; }
    if(k==KEY_RIGHT){ if(caret_pos < current_file->size) caret_pos++; ensure_caret_visible(); return; }

    if(k==KEY_UP){
        int cur_line_start = line_start_of(caret_pos);
        int col = caret_pos - cur_line_start;
        if(cur_line_start==0) return;
        int prev_line_end = cur_line_start-1;
        int prev_line_start = line_start_of(prev_line_end);
        int prev_len = line_length_from(prev_line_start);
        caret_pos = prev_line_start + (col < prev_len ? col : prev_len);
        ensure_caret_visible();
        return;
    }

    if(k==KEY_DOWN){
        int cur_line_start = line_start_of(caret_pos);
        int col = caret_pos - cur_line_start;
        int next = caret_pos;
        // move to end of current line
        while(next < current_file->size && current_file->data[next] != '\n') next++;
        if(next >= current_file->size) return;
        int next_line_start = next+1;
        int next_len = line_length_from(next_line_start);
        caret_pos = next_line_start + (col < next_len ? col : next_len);
        ensure_caret_visible();
        return;
    }

    if(k==KEY_ENTER){
        insert_char_at(caret_pos, '\n');
        caret_pos++;
        ensure_caret_visible();
        return;
    }

    // printable ASCII insertion
    if(k >= 32 && k <= 126){
        insert_char_at(caret_pos, (char)k);
        caret_pos++;
        ensure_caret_visible();
        return;
    }
}

void editor_save(void){
    if(!current_file) return;
    ata_write_sector(current_file->sector, (const uint8_t*)current_file->data);
}

void editor_insert_string(const char* s){
    if(!current_file || !s) return;
    for(size_t i=0;i<kstrlen(s);i++){
        char c = s[i];
        if(c=='\n'){
            insert_char_at(caret_pos, '\n');
            caret_pos++;
        } else {
            insert_char_at(caret_pos, c);
            caret_pos++;
        }
    }
}
