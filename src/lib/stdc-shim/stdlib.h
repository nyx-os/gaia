#ifndef _STDCSHIM_STDLIB_H
#define _STDCSHIM_STDLIB_H
#include <stdint.h>

//! Note: this isn't standard
const char *itoa(int64_t value, char *str, int base);
const char *utoa(uint64_t value, char *str, int base);

#endif
