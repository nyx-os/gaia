#ifndef GAIA_VM_VM_KERNEL_H
#define GAIA_VM_VM_KERNEL_H
#include <kern/vm/vmem.h>

#define KERNEL_HEAP_BASE (P2V(0) + GIB(4))
#define KERNEL_HEAP_SIZE GIB(2)

void vm_kernel_init(void);

void *vm_kernel_alloc(int npages, bool bootstrap);

void vm_kernel_free(void *ptr, int npages);

vmem_stat_t vm_kernel_stat(void);

#endif
