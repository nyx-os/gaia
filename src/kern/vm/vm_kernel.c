#include <machdep/vm.h>
#include <kern/vm/vm.h>
#include <kern/vm/vmem.h>
#include <kern/vm/vm_kernel.h>

void vm_kernel_init(void)
{
    vmem_init(&vm_kmap.vmem, "Kernel Heap", (void *)KERNEL_HEAP_BASE,
              KERNEL_HEAP_SIZE, PAGE_SIZE, NULL, NULL, NULL, 0, 0);
}

void *vm_kernel_alloc(int npages, bool bootstrap)
{
    void *virt = vmem_alloc(&vm_kmap.vmem, npages * PAGE_SIZE,
                            VM_INSTANTFIT | (bootstrap ? VM_BOOTSTRAP : 0));

    for (int i = 0; i < npages; i++) {
        paddr_t addr = phys_allocz();
        pmap_enter(vm_kmap.pmap, (uintptr_t)virt + i * PAGE_SIZE, addr,
                   VM_PROT_WRITE | VM_PROT_READ, PMAP_NONE);
    }

    return virt;
}

void vm_kernel_free(void *ptr, int npages)
{
    vmem_free(&vm_kmap.vmem, ptr, npages * PAGE_SIZE);

    assert(!"TODO");
}

vmem_stat_t vm_kernel_stat(void)
{
    return vm_kmap.vmem.stat;
}
