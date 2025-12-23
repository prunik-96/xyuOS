#include "fs.h"
#include "../util/string.h"

static file_t files[MAX_FILES];
static uint32_t next_sector = 100;   // начиная с сектора 100

void fs_init(void){
    for(int i=0;i<MAX_FILES;i++){
        files[i].used = 0;
        files[i].size = 0;
        files[i].sector = 0;
        files[i].name[0] = 0;
        files[i].data[0] = 0;
    }
}

file_t* fs_create(const char* name){
    for(int i=0;i<MAX_FILES;i++){
        if(!files[i].used){
            files[i].used = 1;

            int n = kstrlen(name);
            if(n >= MAX_FILENAME) n = MAX_FILENAME - 1;
            kmemcpy(files[i].name, name, n);
            files[i].name[n] = 0;

            files[i].size = 0;
            files[i].data[0] = 0;

            files[i].sector = next_sector++;   // 👈 уникальный сектор
            return &files[i];
        }
    }
    return 0;
}

file_t* fs_get(const char* name){
    for(int i=0;i<MAX_FILES;i++){
        if(files[i].used && kstrcmp(files[i].name, name) == 0){
            return &files[i];
        }
    }
    return 0;
}

int fs_count(void){
    int c = 0;
    for(int i=0;i<MAX_FILES;i++)
        if(files[i].used) c++;
    return c;
}

file_t* fs_at(int index){
    int n = 0;
    for(int i=0;i<MAX_FILES;i++){
        if(files[i].used){
            if(n == index) return &files[i];
            n++;
        }
    }
    return 0;
}
