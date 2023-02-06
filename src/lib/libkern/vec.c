/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include "vec.h"

int vec_expand_(char **data, int *length, int *capacity, int memsz)
{
    if (*length + 1 > *capacity) {
        void *ptr;
        int n = (*capacity == 0) ? 1 : *capacity << 1;
        ptr = krealloc(*data, *length * memsz, n * memsz);
        if (ptr == NULL)
            return -1;
        *data = ptr;
        *capacity = n;
    }
    return 0;
}

int vec_reserve_(char **data, int *length, int *capacity, int memsz, int n)
{
    (void)length;
    if (n > *capacity) {
        void *ptr = krealloc(*data, *length * memsz, n * memsz);
        if (ptr == NULL)
            return -1;
        *data = ptr;
        *capacity = n;
    }
    return 0;
}

int vec_reserve_po2_(char **data, int *length, int *capacity, int memsz, int n)
{
    int n2 = 1;
    if (n == 0)
        return 0;
    while (n2 < n)
        n2 <<= 1;
    return vec_reserve_(data, length, capacity, memsz, n2);
}

int vec_compact_(char **data, int *length, int *capacity, int memsz)
{
    if (*length == 0) {
        kfree(*data, *capacity * memsz);
        *data = NULL;
        *capacity = 0;
        return 0;
    } else {
        void *ptr;
        int n = *length;
        ptr = krealloc(*data, *length * memsz, n * memsz);
        if (ptr == NULL)
            return -1;
        *capacity = n;
        *data = ptr;
    }
    return 0;
}
