#pragma once
#include "../util/types.h"

void vga_clear(uint8_t color);
void vga_print(int x, int y, const char* str, uint8_t color);
void vga_box(int x, int y, int w, int h, uint8_t color);
