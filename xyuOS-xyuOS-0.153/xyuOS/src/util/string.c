#include "string.h"

size_t kstrlen(const char* s){ size_t n=0; while(s && s[n]) n++; return n; }

int kstrcmp(const char* a, const char* b){
  size_t i=0;
  while(a[i] && b[i]){
    if(a[i]!=b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
    i++;
  }
  return (unsigned char)a[i] - (unsigned char)b[i];
}

int kstartswith(const char* s, const char* pfx){
  size_t i=0;
  while(pfx[i]){
    if(s[i]!=pfx[i]) return 0;
    i++;
  }
  return 1;
}

void kmemset(void* p, int v, size_t n){
  uint8_t* b=(uint8_t*)p;
  for(size_t i=0;i<n;i++) b[i]=(uint8_t)v;
}

void kmemcpy(void* d, const void* s, size_t n){
  uint8_t* dd=(uint8_t*)d; const uint8_t* ss=(const uint8_t*)s;
  for(size_t i=0;i<n;i++) dd[i]=ss[i];
}

static int hexv(char c){
  if(c>='0'&&c<='9') return c-'0';
  if(c>='a'&&c<='f') return 10+(c-'a');
  if(c>='A'&&c<='F') return 10+(c-'A');
  return -1;
}

int kparse_hex_byte(const char* s, uint8_t* out){
  int a=hexv(s[0]); int b=hexv(s[1]);
  if(a<0||b<0) return 0;
  *out = (uint8_t)((a<<4)|b);
  return 1;
}
