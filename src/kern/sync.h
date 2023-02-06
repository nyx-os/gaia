/* @license:bsd2 */

/**
 * @file
 * @brief Synchronization primitives
   */

#ifndef SRC_KERN_SYNC_H_
#define SRC_KERN_SYNC_H_

#define SPINLOCK_INITIALIZER 0

typedef int spinlock_t;

static inline void spinlock_lock(spinlock_t *lock)
{
    while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
#if defined(__x86_64__)
        __asm__ volatile("pause");
#endif
    }
}

static inline void lock_release(spinlock_t *lock)
{
    __sync_bool_compare_and_swap(lock, 1, 0);
}

#endif
