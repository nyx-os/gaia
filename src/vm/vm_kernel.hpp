/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <lib/base.hpp>
#include <vm/phys.hpp>
#include <vm/vm.hpp>

namespace Gaia::Vm {

#define KERNEL_HEAP_BASE 0xffff810000000000
#define KERNEL_HEAP_SIZE GIB(4)

void vm_kernel_init();
extern "C" void *vm_kernel_alloc(int npages, bool bootstrap = false);
extern "C" void vm_kernel_free(void *ptr, int npages);

void *vm_kernel_alloc_at_phys(size_t npages, uintptr_t phys);

extern Space *kernel_space;

} // namespace Gaia::Vm
