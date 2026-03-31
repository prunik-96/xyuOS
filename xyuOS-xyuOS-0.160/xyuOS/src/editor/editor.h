#ifndef EDITOR_H
#define EDITOR_H

#include "../fs/fs.h"
#include "../drivers/keyboard.h"

void editor_init(void);
void editor_open(file_t* file);
int  editor_is_active(void);
void editor_set_area(int x, int y, int w, int h);
void editor_handle_key(keycode_t k);
void editor_draw(void);
void editor_save(void);
void editor_insert_string(const char* s);

#endif
