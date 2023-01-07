#include "gaia/host.h"
#include <gaia/pmm.h>
#include <gaia/vm/vm_kernel.h>
#include <gaia/vm/vmem.h>
#include <paging.h>

static Vmem vmem_kernel;

void vm_kernel_init(void)
{
    vmem_init(&vmem_kernel, "Kernel Heap", (void *)KERNEL_HEAP_BASE, KERNEL_HEAP_SIZE, PAGE_SIZE, NULL, NULL, NULL, 0, 0);
}

void *vm_kernel_alloc(int npages, bool bootstrap)
{
    void *virt = vmem_alloc(&vmem_kernel, npages * PAGE_SIZE, VM_INSTANTFIT | (bootstrap ? VM_BOOTSTRAP : 0));

    for (int i = 0; i < npages; i++)
    {
        void *addr = pmm_alloc_zero();
        host_map_page(paging_get_kernel_pagemap(), (uintptr_t)virt + i * PAGE_SIZE, (uintptr_t)addr, PAGE_WRITABLE);
    }

    return virt;
}

void vm_kernel_free(void *ptr, int npages)
{
    vmem_free(&vmem_kernel, ptr, npages * PAGE_SIZE);

    for (int i = 0; i < npages; i++)
    {
        pmm_free((void *)paging_virt_to_phys(paging_get_kernel_pagemap(), (uintptr_t)ptr + i * PAGE_SIZE));
        host_unmap_page(paging_get_kernel_pagemap(), (uintptr_t)ptr + i * PAGE_SIZE);
    }
}

VmemStat vm_kernel_stat(void)
{
    return vmem_kernel.stat;
}
