#include "frg/manual_box.hpp"
#include "kernel/ipl.hpp"
#include "lib/spinlock.hpp"
#include <frg/slab.hpp>
#include <hal/hal.hpp>
#include <lib/log.hpp>
#include <vm/heap.hpp>
#include <vm/vm_kernel.hpp>

namespace Gaia::Vm {
struct HeapStats {
  size_t allocated;
};

#if HEAP_ACCOUNTING_ENABLED
static HeapStats heap_stats[5];
#endif

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

static const char *subsystem_names[] = {
    "Unknown", "sched", "dev", "fs", "vm",
};

static frg::manual_box<Spinlock> lock;

void dump_heap_stats() {
#if HEAP_ACCOUNTING_ENABLED
  if (!lock.valid()) {
    lock.initialize();
  }

  lock->lock();
  auto ipl = iplx(Ipl::HIGH);
  for (size_t i = 0; i < frg::array_size(heap_stats); i++) {
    log("{}: {} bytes allocated", subsystem_names[i], heap_stats[i].allocated);
  }
  iplx(ipl);
  lock->unlock();
#endif
}

} // namespace Gaia::Vm

void *operator new(size_t size, Gaia::Vm::Subsystem subsystem) {
#if HEAP_ACCOUNTING_ENABLED
  Gaia::Vm::heap_stats[subsystem].allocated += size;
#endif
  return Gaia::Vm::malloc(size);
}

void *operator new[](size_t size, Gaia::Vm::Subsystem subsystem) {
#if HEAP_ACCOUNTING_ENABLED
  Gaia::Vm::heap_stats[subsystem].allocated += size;
#endif
  return Gaia::Vm::malloc(size);
}

void *operator new(size_t size) {
#if HEAP_ACCOUNTING_ENABLED
  Gaia::Vm::heap_stats[Gaia::Vm::Subsystem::UNKNOWN].allocated += size;
#endif
  return Gaia::Vm::malloc(size);
}
void *operator new[](size_t size) {
#if HEAP_ACCOUNTING_ENABLED
  Gaia::Vm::heap_stats[Gaia::Vm::Subsystem::UNKNOWN].allocated += size;
#endif
  return Gaia::Vm::malloc(size);
}

void operator delete(void *ptr) { return Gaia::Vm::free(ptr); }
void operator delete[](void *ptr) { return Gaia::Vm::free(ptr); }
