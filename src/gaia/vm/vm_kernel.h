#ifndef GAIA_VM_VM_KERNEL_H
#define GAIA_VM_VM_KERNEL_H
#include <gaia/base.h>
#include <gaia/vm/vmem.h>

#define KERNEL_HEAP_BASE (host_phys_to_virt(0) + GIB(4))
#define KERNEL_HEAP_SIZE GIB(4)

void vm_kernel_init(void);

void *vm_kernel_alloc(int npages, bool bootstrap);

void vm_kernel_free(void *ptr, int npages);

VmemStat vm_kernel_stat(void);

#endif
