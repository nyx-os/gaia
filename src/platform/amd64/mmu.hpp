/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <stddef.h>

#define PML_ENTRY(addr, offset)                                                \
  (size_t)(addr & ((uintptr_t)0x1ff << offset)) >> offset;

#define PTE_PRESENT (1ull << 0ull)
#define PTE_WRITABLE (1ull << 1ull)
#define PTE_USER (1ull << 2ull)
#define PTE_NOT_EXECUTABLE (1ull << 63ull)
#define PTE_HUGE (1ull << 7ull)

#define PTE_ADDR_MASK 0x000ffffffffff000
#define PTE_GET_ADDR(VALUE) ((VALUE)&PTE_ADDR_MASK)
#define PTE_GET_FLAGS(VALUE) ((VALUE) & ~PTE_ADDR_MASK)

#define PTE_IS_PRESENT(x) (((x)&PTE_PRESENT) != 0)
#define PTE_IS_WRITABLE(x) (((x)&PTE_WRITABLE) != 0)
#define PTE_IS_HUGE(x) (((x)&PTE_HUGE) != 0)
#define PTE_IS_USER(x) (((x)&PTE_USER) != 0)
#define PTE_IS_NOT_EXECUTABLE(x) (((x)&PTE_NOT_EXECUTABLE) != 0)

namespace Gaia::Hal {
constexpr size_t PAGE_SIZE = 4096;
}
