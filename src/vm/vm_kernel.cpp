#include "lib/log.hpp"
#include <frg/manual_box.hpp>
#include <frg/spinlock.hpp>
#include <hal/hal.hpp>
#include <vm/phys.hpp>
#include <vm/vm.hpp>
#include <vm/vm_kernel.hpp>
#include <vm/vmem.h>

namespace Gaia::Vm {

using namespace Hal::Vm;

static frg::manual_box<Vmem> vmem_kernel;
static frg::manual_box<frg::simple_spinlock> spinlock;
Space *kernel_space;

extern "C" void vmem_lock() {
  if (!spinlock.valid()) {
    spinlock.initialize();
  }
  spinlock->lock();
}

extern "C" void vmem_unlock() { spinlock->unlock(); }

void vm_kernel_init() {

  if (!spinlock.valid())
    spinlock.initialize();

  vmem_kernel.initialize();

  vmem_init(vmem_kernel.get(), "Kernel Heap",
            reinterpret_cast<void *>(KERNEL_HEAP_BASE), KERNEL_HEAP_SIZE,
            Hal::PAGE_SIZE, NULL, NULL, NULL, 0, 0);

  kernel_space = new Space(&kernel_pagemap, *vmem_kernel.get());
}

extern "C" void *vm_kernel_alloc(int npages, bool bootstrap) {
  void *virt = vmem_alloc(vmem_kernel.get(), npages * Hal::PAGE_SIZE,
                          VM_INSTANTFIT | (bootstrap ? VM_BOOTSTRAP : 0));

  for (int i = 0; i < npages; i++) {
    void *addr = phys_alloc(true).unwrap();

    kernel_pagemap.map((uintptr_t)virt + i * Hal::PAGE_SIZE, (uintptr_t)addr,
                       (Prot)(Prot::READ | Prot::WRITE), Hal::Vm::Flags::NONE);
  }

  return virt;
}

void *vm_kernel_alloc_at_phys(size_t npages, uintptr_t phys) {
  void *virt =
      vmem_alloc(vmem_kernel.get(), npages * Hal::PAGE_SIZE, VM_INSTANTFIT);

  for (size_t i = 0; i < npages; i++) {
    kernel_pagemap.map((uintptr_t)virt + i * Hal::PAGE_SIZE,
                       (uintptr_t)phys + i * Hal::PAGE_SIZE,
                       (Prot)(Prot::READ | Prot::WRITE), Hal::Vm::Flags::NONE);
  }

  return virt;
}

extern "C" void vm_kernel_free(void *ptr, int npages) {
  vmem_free(vmem_kernel.get(), ptr, npages * Hal::PAGE_SIZE);

  if (*(uint64_t *)ptr == 0xcccccccccccccccc) {
    panic("possible double-free: {:x}", ptr);
  }

  // Prevent use-after-free
  memset(ptr, 0xcc, npages * Hal::PAGE_SIZE);

  for (int i = 0; i < npages; i++) {
    auto virt = (uintptr_t)ptr + i * Hal::PAGE_SIZE;

    auto phys = kernel_pagemap.get_mapping(virt);

    if (!phys.is_ok()) {
      panic("couldnt find mapping for virt {:x}", virt);
    }

    phys_free(reinterpret_cast<void *>(phys.unwrap().address));

    kernel_pagemap.unmap(virt);
  }
}

} // namespace Gaia::Vm
