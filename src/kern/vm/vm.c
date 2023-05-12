/* SPDX-License-Identifier: BSD-2-Clause */
#include "kern/vm/vmem.h"
#include <kern/vm/vm.h>
#include <kern/vm/vm_kernel.h>

vm_map_t vm_kmap;

void vm_init(charon_t charon)
{
    phys_init(charon);

    pmap_init();

    vmem_bootstrap();

    vm_kernel_init();

    kmem_bootstrap();
}

int vm_map(vm_map_t *map, vaddr_t *vaddrp, size_t size, vm_prot_t prot,
           vaddr_t *out)
{
    vaddr_t vaddr = 0;

    if (vaddrp) {
        vaddr = *vaddrp;
    } else {
        vaddr = (vaddr_t)vmem_alloc(&map->vmem, size, VM_INSTANTFIT);
    }

    for (size_t i = 0; i < DIV_CEIL(size, PAGE_SIZE); i++) {
        pmap_enter(map->pmap, vaddr + i * PAGE_SIZE, phys_allocz(), prot,
                   PMAP_USER);
    }

    *out = vaddr;

    return 0;
}
