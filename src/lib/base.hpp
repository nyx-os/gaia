/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file base.hpp
 *
 * @brief General utilities
 */
#pragma once
#include <frg/list.hpp>
#include <stddef.h>
#include <stdint.h>
#include <utility>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#define DIV_CEIL(x, align) (((x) + (align)-1) / (align))

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define KIB(x) ((uint64_t)x << 10)
#define MIB(x) ((uint64_t)x << 20)
#define GIB(x) ((uint64_t)x << 30)

// should be moved elsewhere

#ifdef __KERNEL__
constexpr int toupper(int c) {
  if (c >= 'a' && c <= 'z') {
    c -= 0x20;
  }
  return c;
}
#endif

namespace Gaia {

/**
 * @brief Writes to an address
 *
 * @tparam T The type of data to write
 * @param address The address to write to
 * @param data What to write
 */
template <typename T> static inline void volatile_write(T *address, T data) {
  *static_cast<volatile T *>(address) = data;
}

/**
 * @brief Reads from an address
 *
 * @tparam T The type of data to read
 * @param address The address to read from
 * @returns The data that was read
 */
template <typename T> static inline T volatile_read(T *address) {
  return *static_cast<volatile T *>(address);
}

} // namespace Gaia

#if !__has_include(<string.h>)
#define memcpy __builtin_memcpy
extern "C" {
void *memset(void *d, int c, size_t n);
size_t strlen(const char *s);
}
#endif
