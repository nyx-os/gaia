/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/slab.hpp>
#include <frg/spinlock.hpp>
#include <frg/string.hpp>
#include <frg/unique.hpp>
#include <frg/vector.hpp>
#include <lib/base.hpp>

namespace Gaia::Vm {
extern "C" void *malloc(size_t bytes);
extern "C" void free(void *ptr);
extern "C" void *realloc(void *ptr, size_t newsize);

struct VirtualAllocator {
public:
  uintptr_t map(size_t length);

  void unmap(uintptr_t address, size_t length);
};

using MemoryAllocator =
    frg::slab_allocator<VirtualAllocator, frg::simple_spinlock>;

using MemoryPool = frg::slab_pool<VirtualAllocator, frg::simple_spinlock>;

struct HeapAllocator {
  static void *allocate(size_t size) { return operator new(size); }

  static void deallocate(void *ptr, size_t size) {
    (void)size;
    free(ptr);
  }

  static void free(void *ptr) { operator delete(ptr); }
};

MemoryAllocator &get_allocator();

using String = frg::string<HeapAllocator>;
template <typename T> using Vector = frg::vector<T, HeapAllocator>;
template <typename T> using UniquePtr = frg::unique_ptr<T, MemoryAllocator>;

} // namespace Gaia::Vm
