#pragma once
#include "../drivers/keyboard.h"
#include <stdint.h>

#define EDITOR_MAX_LINES 30
#define EDITOR_LINE_WIDTH 50
#define EDITOR_WIDTH 36
#define EDITOR_HEIGHT 14

typedef struct {
  // Буфер текста
  char data[EDITOR_MAX_LINES][EDITOR_LINE_WIDTH];
  
  // Позиция курсора
  int cur_x;
  int cur_y;
  
  // Скроллинг
  int scroll_y;
  
  // Метаинформация
  int lines;
  int modified;
  char filename[64];
} editor_t;

// Функции управления редактором
void editor_init(editor_t* ed);
void editor_handle_key(editor_t* ed, keycode_t key, char ch);
void editor_render(editor_t* ed);

// Функции сохранения/загрузки
int editor_save(editor_t* ed, const char* path);
int editor_load(editor_t* ed, const char* path);

// Утилиты
char* editor_get_line(editor_t* ed, int line);
int editor_get_lines(editor_t* ed);