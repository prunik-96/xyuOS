#pragma once
#include "../util/types.h"

void vga_clear(uint8_t color);
void vga_print(int x, int y, const char* str, uint8_t color);
void vga_box(int x, int y, int w, int h, uint8_t color);
// Enable/disable blue full-screen theme (like BSOD). When enabled the
// background is forced to blue while foreground colors are preserved.
void vga_set_blue_theme(int enable);
// Low-level cell access (raw): get/put a 16-bit VGA cell at coordinates.
uint16_t vga_getcell(int x, int y);
void vga_putcell(int x, int y, uint16_t cell);
