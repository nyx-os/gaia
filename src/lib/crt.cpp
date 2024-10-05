/* SPDX-License-Identifier: BSD-2-Clause */
#include <lib/log.hpp>
#include <stddef.h>
#include <stdint.h>

extern "C" void *memset(void *d, int c, size_t n) noexcept {
#ifdef __x86_64__
  asm volatile("rep stosb" : "+D"(d), "+c"(n) : "al"(c) : "memory");
#else
  char *p = (char *)d;
  while (n--) {
    *p++ = c;
  }
#endif
  return d;
}

extern "C" void *memcpy(void *dest, const void *src, size_t n) noexcept {
#ifdef __x86_64__
  asm volatile("rep movsb" : : "D"(dest), "S"(src), "c"(n));
#else
  char *p1 = (char *)dest;
  char *p2 = (char *)src;

  while (n--) {
    *p1++ = *p2++;
  }
#endif
  return dest;
}

extern "C" void *memmove(void *dest, const void *src, size_t n) noexcept {
  uint8_t *pdest = (uint8_t *)dest;
  const uint8_t *psrc = (const uint8_t *)src;

  if (src > dest) {
    return memcpy(dest, src, n);
  } else if (src < dest) {
    for (size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }

  return dest;
}

extern "C" int memcmp(const void *str1, const void *str2,
                      size_t count) noexcept {
  const unsigned char *c1, *c2;

  c1 = (const unsigned char *)str1;
  c2 = (const unsigned char *)str2;

  while (count-- > 0) {
    if (*c1++ != *c2++)
      return c1[-1] < c2[-1] ? -1 : 1;
  }
  return 0;
}

extern "C" size_t strlen(const char *s) {
  size_t i = 0;
  while (*s++)
    i++;
  return i;
}

extern "C" char *strncpy(char *destination, const char *source, size_t num) {
  while ((*destination++ = *source++) && num--)
    ;
  return destination;
}

/* ------------------------- This section is stolen from mlibc copyright to the
 * original authors ------------- */
/* https://github.com/managarm/mlibc/blob/master/options/internal/gcc/guard-abi.cpp#L48
 */

// Itanium ABI static initialization guard.
struct Guard {
  // bit of the mutex member variable.
  // indicates that the mutex is locked.
  static constexpr int32_t locked = 1;

  void lock() {
    uint32_t v = 0;
    if (__atomic_compare_exchange_n(&mutex, &v, Guard::locked, false,
                                    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
      return;

    __builtin_trap();
  }

  void unlock() { __atomic_store_n(&mutex, 0, __ATOMIC_RELEASE); }

  // the first byte's meaning is fixed by the ABI.
  // it indicates whether initialization has already been completed.
  uint8_t complete;

  // we use some of the remaining bytes to implement a mutex.
  uint32_t mutex;
};

static_assert(sizeof(Guard) == sizeof(int64_t));

/**
 * This function is called before the static variable is initialized.
 * It returns 1 if the static variable should be initialized, 0 if it
 * should not be initialized.
 */
extern "C" int __cxa_guard_acquire(void *ctx) {
  auto guard = reinterpret_cast<Guard *>(ctx);
  guard->lock();
  // relaxed ordering is sufficient because
  // Guard::complete is only modified while the mutex is held.
  if (__atomic_load_n(&guard->complete, __ATOMIC_RELAXED)) {
    guard->unlock();
    return 0;
  } else {
    return 1;
  }
}

/**
 * This function is called after the static variable is initialized.
 */
extern "C" void __cxa_guard_release(void *ctx) {
  auto guard = reinterpret_cast<Guard *>(ctx);
  // do a store-release so that compiler generated code can skip calling
  // __cxa_guard_acquire by doing a load-acquire on Guard::complete.
  __atomic_store_n(&guard->complete, 1, __ATOMIC_RELEASE);
  guard->unlock();
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }

extern "C" void __cxa_pure_virtual() {
  Gaia::panic("pure virtual func called");
  while (1)
    ;
}
