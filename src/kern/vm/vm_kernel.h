/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file vm_kernel.h
 * @brief Kernel virtual page allocator
 *
 * A vmem-backed virtual page allocator used by kmem and vmem itself.
 */
#ifndef GAIA_VM_VM_KERNEL_H
#define GAIA_VM_VM_KERNEL_H
#include <kern/vm/vmem.h>
#include <kern/vm/phys.h>

#define KERNEL_HEAP_BASE \
    ((uintptr_t)(P2V(0) + MAX(GIB(4), phys_total_pages() * PAGE_SIZE)))

#define KERNEL_HEAP_SIZE GIB(2)

/**
 * Initializes the subsystem
 */
void vm_kernel_init(void);

/**
 * Allocates a virtually-contiguous span of pages
 * @param npages The number of pages to allocate
 * @param bootstrap Whether or not this is allocation is used by vmem
 * @return Pointer to allocated memory
 */
void *vm_kernel_alloc(int npages, bool bootstrap);

/**
 * Frees a previously-allocated pointer
 * @param ptr The pointer to free
 * @param npages The number of pages to free
 */
void vm_kernel_free(void *ptr, int npages);

/**
 * @return Statistics about the
 */
vmem_stat_t vm_kernel_stat(void);

#endif
