#include <frg/slab.hpp>
#include <hal/hal.hpp>
#include <lib/log.hpp>
#include <vm/heap.hpp>
#include <vm/vm_kernel.hpp>

namespace Gaia::Vm {

uintptr_t VirtualAllocator::map(size_t length) {
  return (uintptr_t)vm_kernel_alloc(length / Hal::PAGE_SIZE, false);
}

void VirtualAllocator::unmap(uintptr_t address, size_t length) {
  vm_kernel_free(reinterpret_cast<void *>(address), length / Hal::PAGE_SIZE);
}

MemoryAllocator &get_allocator() {
  static VirtualAllocator virtual_alloc;
  static MemoryPool heap{virtual_alloc};
  static MemoryAllocator ret{&heap};
  return ret;
}

void *malloc(size_t size) { return get_allocator().allocate(size); }

void free(void *ptr) { return get_allocator().free(ptr); }

void *realloc(void *ptr, size_t size) {
  return get_allocator().reallocate(ptr, size);
}

} // namespace Gaia::Vm

void *operator new(size_t size) { return Gaia::Vm::malloc(size); }
void *operator new[](size_t size) { return Gaia::Vm::malloc(size); }

void operator delete(void *ptr) { return Gaia::Vm::free(ptr); }
void operator delete[](void *ptr) { return Gaia::Vm::free(ptr); }