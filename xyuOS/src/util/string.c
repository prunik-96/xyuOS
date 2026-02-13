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

// Новые функции

void uint32_to_str(uint32_t value, char* buf, size_t bufsize){
  if(bufsize < 2) return;
  char tmp[16];
  int i = 0;
  if(value == 0){
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  uint32_t v = value;
  while(v > 0 && i < 15){
    tmp[i++] = '0' + (v % 10);
    v /= 10;
  }
  int j = 0;
  while(i > 0 && j < (int)bufsize - 1){
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';
}

void kstrcat(char* dest, const char* src){
  size_t dest_len = kstrlen(dest);
  size_t src_len = kstrlen(src);
  size_t i = 0;
  while(i < src_len){
    dest[dest_len + i] = src[i];
    i++;
  }
  dest[dest_len + src_len] = '\0';
}

void kstrcpy(char* dest, const char* src, size_t maxlen){
  size_t i = 0;
  while(i < maxlen - 1 && src[i]){
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}
