#include "vga.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;

static uint16_t cell(char c, uint8_t col) {
    return (uint16_t)c | ((uint16_t)col << 8);
}

void vga_clear(uint8_t color) {
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 80; x++)
            VGA[y * 80 + x] = cell(' ', color);
}

void vga_print(int x, int y, const char* str, uint8_t color) {
    for (int i = 0; str[i]; i++)
        VGA[y * 80 + x + i] = cell(str[i], color);
}

void vga_box(int x, int y, int w, int h, uint8_t c) {
    for (int i = 0; i < w; i++) {
        VGA[y * 80 + x + i] = cell('-', c);
        VGA[(y + h - 1) * 80 + x + i] = cell('-', c);
    }
    for (int i = 0; i < h; i++) {
        VGA[(y + i) * 80 + x] = cell('|', c);
        VGA[(y + i) * 80 + x + w - 1] = cell('|', c);
    }

    VGA[y * 80 + x] = cell('+', c);
    VGA[y * 80 + x + w - 1] = cell('+', c);
    VGA[(y + h - 1) * 80 + x] = cell('+', c);
    VGA[(y + h - 1) * 80 + x + w - 1] = cell('+', c);
}
