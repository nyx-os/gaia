#ifndef SRC_LIB_LIBKERN_STRING_H_
#define SRC_LIB_LIBKERN_STRING_H_
#include <stdint.h>
#include <stddef.h>

char *strncpy(char *destination, const char *source, size_t num);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
char *strrchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);

int toupper(int c);

#endif
