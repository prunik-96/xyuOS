#include "editor.h"
#include "../drivers/vga.h"
#include "../util/string.h"

void editor_init(editor_t* ed){
  ed->lines = 1;
  ed->cur_x = 0;
  ed->cur_y = 0;
  ed->scroll_y = 0;
  ed->modified = 0;
  kstrcpy(ed->filename, "untitled.txt", sizeof(ed->filename));
  
  // Инициализируем первую строку
  for(int i = 0; i < EDITOR_MAX_LINES; i++){
    ed->data[i][0] = '\0';
  }
}

void editor_handle_key(editor_t* ed, keycode_t key, char ch){
  if(key == KEY_ENTER){
    // Вставить новую строку
    if(ed->cur_y + 1 < EDITOR_MAX_LINES){
      // Копируем текст после курсора на новую строку
      int cur_line_len = kstrlen(ed->data[ed->cur_y]);
      if(ed->cur_x < cur_line_len){
        kstrcpy(ed->data[ed->cur_y + 1], 
                ed->data[ed->cur_y] + ed->cur_x, 
                EDITOR_LINE_WIDTH);
        ed->data[ed->cur_y][ed->cur_x] = '\0';
      }
      ed->lines++;
      ed->cur_y++;
      ed->cur_x = 0;
      ed->modified = 1;
    }
  }
  else if(key == KEY_BKSP){
    // Backspace
    if(ed->cur_x > 0){
      int line_len = kstrlen(ed->data[ed->cur_y]);
      // Сдвигаем символы влево
      for(int i = ed->cur_x; i < line_len; i++){
        ed->data[ed->cur_y][i - 1] = ed->data[ed->cur_y][i];
      }
      ed->data[ed->cur_y][line_len - 1] = '\0';
      ed->cur_x--;
      ed->modified = 1;
    }
    else if(ed->cur_y > 0){
      // Присоединяем к предыдущей строке
      int prev_len = kstrlen(ed->data[ed->cur_y - 1]);
      if(prev_len + kstrlen(ed->data[ed->cur_y]) < EDITOR_LINE_WIDTH){
        kstrcat(ed->data[ed->cur_y - 1], ed->data[ed->cur_y]);
        // Удаляем текущую строку
        for(int i = ed->cur_y; i < ed->lines - 1; i++){
          kstrcpy(ed->data[i], ed->data[i + 1], EDITOR_LINE_WIDTH);
        }
        ed->data[ed->lines - 1][0] = '\0';
        ed->lines--;
        ed->cur_y--;
        ed->cur_x = prev_len;
        ed->modified = 1;
      }
    }
  }
  else if(key == KEY_LEFT){
    if(ed->cur_x > 0) ed->cur_x--;
    else if(ed->cur_y > 0){
      ed->cur_y--;
      ed->cur_x = kstrlen(ed->data[ed->cur_y]);
    }
  }
  else if(key == KEY_RIGHT){
    int line_len = kstrlen(ed->data[ed->cur_y]);
    if(ed->cur_x < line_len) ed->cur_x++;
    else if(ed->cur_y + 1 < ed->lines){
      ed->cur_y++;
      ed->cur_x = 0;
    }
  }
  else if(key == KEY_UP){
    if(ed->cur_y > 0){
      ed->cur_y--;
      int new_len = kstrlen(ed->data[ed->cur_y]);
      if(ed->cur_x > new_len) ed->cur_x = new_len;
    }
  }
  else if(key == KEY_DOWN){
    if(ed->cur_y + 1 < ed->lines){
      ed->cur_y++;
      int new_len = kstrlen(ed->data[ed->cur_y]);
      if(ed->cur_x > new_len) ed->cur_x = new_len;
    }
  }
  else if(ch != 0){
    // Вставить символ
    int line_len = kstrlen(ed->data[ed->cur_y]);
    if(line_len + 1 < EDITOR_LINE_WIDTH){
      // Сдвигаем символы вправо
      for(int i = line_len; i >= ed->cur_x; i--){
        ed->data[ed->cur_y][i + 1] = ed->data[ed->cur_y][i];
      }
      ed->data[ed->cur_y][ed->cur_x] = ch;
      ed->cur_x++;
      ed->modified = 1;
    }
  }
  
  // Обновляем scroll_y если нужно
  if(ed->cur_y < ed->scroll_y) ed->scroll_y = ed->cur_y;
  if(ed->cur_y >= ed->scroll_y + EDITOR_HEIGHT){
    ed->scroll_y = ed->cur_y - EDITOR_HEIGHT + 1;
  }
}

void editor_render(editor_t* ed){
  // Очистить область редактора (1,1 до 38,17)
  for(int y = 0; y < EDITOR_HEIGHT; y++){
    for(int x = 0; x < EDITOR_WIDTH; x++){
      vga_print(2 + x, 3 + y, " ", 0x0F);
    }
  }
  
  // Отрисовать строки текста
  for(int i = 0; i < EDITOR_HEIGHT && ed->scroll_y + i < ed->lines; i++){
    const char* line = ed->data[ed->scroll_y + i];
    
    // Отрисовать символы строки
    for(int j = 0; j < EDITOR_WIDTH && line[j] != '\0'; j++){
      uint8_t color = 0x0F; // белый на черном
      
      // Если это позиция курсора, подсвечиваем
      if(ed->scroll_y + i == ed->cur_y && j == ed->cur_x){
        color = 0xF0; // черный на белом (инверсия)
      }
      
      char ch[2] = {line[j], '\0'};
      vga_print(2 + j, 3 + i, ch, color);
    }
    
    // Отрисовать курсор в конце строки
    if(ed->scroll_y + i == ed->cur_y && ed->cur_x == kstrlen(line)){
      vga_print(2 + ed->cur_x, 3 + i, "_", 0xF0);
    }
  }
  
  // Показать статус
  char status[40];
  if(ed->modified){
    kstrcpy(status, "* Modified", sizeof(status));
  } else {
    kstrcpy(status, "Saved", sizeof(status));
  }
  vga_print(2, 19, status, 0x07);
  
  // Показать позицию курсора
  char pos[20];
  char buf_y[8], buf_x[8];
  uint32_to_str(ed->cur_y + 1, buf_y);
  uint32_to_str(ed->cur_x + 1, buf_x);
  kstrcpy(pos, "Ln:", sizeof(pos));
  kstrcat(pos, buf_y);
  kstrcat(pos, " Col:", sizeof(pos));
  kstrcat(pos, buf_x);
  vga_print(20, 19, pos, 0x07);
}

int editor_save(editor_t* ed, const char* path){
  // TODO: реализовать сохранение файла
  // Пока просто отмечаем как сохраненный
  ed->modified = 0;
  kstrcpy(ed->filename, path, sizeof(ed->filename));
  return 1;
}

int editor_load(editor_t* ed, const char* path){
  // TODO: реализовать загрузку файла
  return 0;
}

char* editor_get_line(editor_t* ed, int line){
  if(line >= 0 && line < ed->lines){
    return ed->data[line];
  }
  return "";
}

int editor_get_lines(editor_t* ed){
  return ed->lines;
}