#ifndef EDITOR_H
#define EDITOR_H

#include "../fs/fs.h"

void editor_init();
void editor_open(file_t* file);
void editor_handle_key(char c);
void editor_draw();
void editor_save();

#endif
