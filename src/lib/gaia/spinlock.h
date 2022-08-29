/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef LIB_GAIA_SPINLOCK_H
#define LIB_GAIA_SPINLOCK_H

typedef int Spinlock;

void lock_acquire(Spinlock *lock);
void lock_release(Spinlock *lock);

#endif /* LIB_GAIA_SPINLOCK_H */
