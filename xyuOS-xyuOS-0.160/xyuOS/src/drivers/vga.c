#include "vga.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;

static uint16_t cell(char c, uint8_t col) {
    return (uint16_t)c | ((uint16_t)col << 8);
}

static int blue_theme = 0;
static uint8_t blue_bg = 1;   // blue background palette index
static uint8_t blue_fg = 15;  // default foreground when needed (white)

void vga_set_blue_theme(int enable){
    blue_theme = enable ? 1 : 0;
}

static uint8_t apply_theme(uint8_t col){
    if(!blue_theme) return col;
    uint8_t fg = col & 0x0F;
    // When theme enabled, force background to blue but keep fg.
    return (uint8_t)((blue_bg<<4) | (fg & 0x0F));
}

uint16_t vga_getcell(int x, int y){
    if(x < 0 || x >= 80 || y < 0 || y >= 25) return 0;
    return VGA[y * 80 + x];
}

void vga_putcell(int x, int y, uint16_t cell){
    if(x < 0 || x >= 80 || y < 0 || y >= 25) return;
    VGA[y * 80 + x] = cell;
}

void vga_clear(uint8_t color) {
    uint8_t attr = apply_theme(color);
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            VGA[y * 80 + x] = cell(' ', attr);
}

void vga_print(int x, int y, const char* str, uint8_t color) {
    uint8_t attr = apply_theme(color);
    for (int i = 0; str[i]; i++)
        VGA[y * 80 + x + i] = cell(str[i], attr);
}

void vga_box(int x, int y, int w, int h, uint8_t c) {
    uint8_t attr = apply_theme(c);
    for (int i = 0; i < w; i++) {
        VGA[y * 80 + x + i] = cell('-', attr);
        VGA[(y + h - 1) * 80 + x + i] = cell('-', attr);
    }
    for (int i = 0; i < h; i++) {
        VGA[(y + i) * 80 + x] = cell('|', attr);
        VGA[(y + i) * 80 + x + w - 1] = cell('|', attr);
    }

    VGA[y * 80 + x] = cell('+', attr);
    VGA[y * 80 + x + w - 1] = cell('+', attr);
    VGA[(y + h - 1) * 80 + x] = cell('+', attr);
    VGA[(y + h - 1) * 80 + x + w - 1] = cell('+', attr);
}
