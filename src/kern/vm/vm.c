/* SPDX-License-Identifier: BSD-2-Clause */
#include <kern/vm/vm.h>
#include <kern/vm/phys.h>

void vm_init(charon_t charon)
{
    phys_init(charon);
    pmap_init();
}
