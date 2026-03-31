#pragma once
#include <stdint.h>
#include <stddef.h>

size_t kstrlen(const char* s);
int    kstrcmp(const char* a, const char* b);
int    kstartswith(const char* s, const char* pfx);
void   kmemset(void* p, int v, size_t n);
void   kmemcpy(void* d, const void* s, size_t n);

int    kparse_hex_byte(const char* s, uint8_t* out); // "90" -> 0x90
