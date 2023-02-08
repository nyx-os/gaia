/* SPDX-License-Identifier: BSD-2-Clause */
#include <kern/vm/vm.h>
#include <kern/vm/phys.h>
#include <kern/vm/vmem.h>
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
