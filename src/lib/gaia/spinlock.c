/*
 * Copyright (c) 2022, lg
 * 
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/spinlock.h>

void lock_acquire(Spinlock *lock)
{
    while (!__sync_bool_compare_and_swap(&lock->_lock, 0, 1))
    {

#if defined(__x86_64__)
        __asm__ volatile("pause");
#endif
    }
}

void lock_release(Spinlock *lock)
{
    __sync_bool_compare_and_swap(&lock->_lock, 1, 0);
}
