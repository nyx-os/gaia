/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef LIB_GAIA_BASE_H
#define LIB_GAIA_BASE_H

#include <gaia/debug.h>
#include <gaia/spinlock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#define DIV_CEIL(x, align) (((x) + (align)-1) / (align))
#define PACKED __attribute__((packed))
#define DISCARD(x) (void)x

#endif /* LIB_GAIA_BASE_H */
