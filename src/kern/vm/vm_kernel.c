#include <machdep/vm.h>
#include <kern/vm/vm.h>
#include <kern/vm/vmem.h>
#include <kern/vm/vm_kernel.h>

static vmem_t vmem_kernel;

void vm_kernel_init(void)
{
    vmem_init(&vmem_kernel, "Kernel Heap", (void *)KERNEL_HEAP_BASE,
              KERNEL_HEAP_SIZE, PAGE_SIZE, NULL, NULL, NULL, 0, 0);
}

void *vm_kernel_alloc(int npages, bool bootstrap)
{
    void *virt = vmem_alloc(&vmem_kernel, npages * PAGE_SIZE,
                            VM_INSTANTFIT | (bootstrap ? VM_BOOTSTRAP : 0));

    for (int i = 0; i < npages; i++) {
        paddr_t addr = phys_allocz();
        pmap_enter(kernel_pmap, (uintptr_t)virt + i * PAGE_SIZE, addr,
                   VM_PROT_WRITE | VM_PROT_READ, PMAP_NONE);
    }

    return virt;
}

void vm_kernel_free(void *ptr, int npages)
{
    vmem_free(&vmem_kernel, ptr, npages * PAGE_SIZE);

    assert(!"TODO");
}

vmem_stat_t vm_kernel_stat(void)
{
    return vmem_kernel.stat;
}
