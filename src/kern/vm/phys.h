/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_KERN_VM_PHYS_H_
#define SRC_KERN_VM_PHYS_H_
#include <kern/charon.h>
#include <machdep/vm.h>

void phys_init(charon_t charon);

paddr_t phys_alloc(void);
paddr_t phys_allocz(void);
void phys_free(paddr_t addr);

size_t phys_used_pages(void);
size_t phys_total_pages(void);

#endif
