/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_SLAB_H
#define SRC_GAIA_SLAB_H

/** @file
 *  @brief Not-caching slab allocator.
 */

#include <gaia/base.h>

#define SLAB_COUNT 7

void *slab_alloc(size_t size);
void slab_free(void *ptr);
void *slab_realloc(void *ptr, size_t size);
void slab_init(void);
size_t slab_used(void);
void slab_dump(void);

#endif /* SRC_GAIA_SLAB_H */
