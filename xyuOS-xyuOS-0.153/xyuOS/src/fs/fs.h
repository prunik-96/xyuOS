#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MAX_FILES 32
#define MAX_FILE_SIZE 512   
#define MAX_FILENAME 32

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
    uint32_t sector;      
} file_t;

void fs_init(void);
file_t* fs_create(const char* name);
file_t* fs_get(const char* name);

int fs_count(void);
file_t* fs_at(int index);

#endif
