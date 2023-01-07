/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/slab.h>
#include <gaia/vec.h>
#include <stdc-shim/string.h>

void vec_expand(void **data, size_t *length, size_t *capacity, int memsz)
{
    if (*length == *capacity)
    {
        *capacity = (*capacity == 0) ? 1 : (*capacity * 2);

        *data = realloc(*data, *capacity * memsz);
    }
}
