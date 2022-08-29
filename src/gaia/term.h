/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_TERM_H
#define SRC_GAIA_TERM_H
#include <gaia/charon.h>

void term_init(Charon *charon);
void term_write(const char *str);

#endif /* SRC_GAIA_TERM_H */
