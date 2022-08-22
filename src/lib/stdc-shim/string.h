#ifndef _STDCSHIM_STRING_H
#define _STDCSHIM_STRING_H
#include <stddef.h>

// A subset of C's string.h
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
char *strrev(char *s);
char *strrchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);

#endif
