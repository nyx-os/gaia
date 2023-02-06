/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_KERN_VM_VM_H_
#define SRC_KERN_VM_VM_H_
#include <machdep/vm.h>
#include <kern/vm/phys.h>

typedef enum {
    VM_PROT_READ = 1,
    VM_PROT_WRITE = 2,
    VM_PROT_EXECUTE = 4,
} vm_prot_t;

typedef enum {
    PMAP_NONE,
    PMAP_HUGE,
    PMAP_LARGE,
} pmap_flags_t;

typedef struct pmap pmap_t;

void pmap_init(void);

pmap_t pmap_create(void);

void pmap_destroy(pmap_t pmap);

/*
 * Create a mapping in physical map pmap for the physical address pa at the virtual address va with protection prot
 */
int pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
               pmap_flags_t flags);

/*
 * Remove mapping.
 */
void pmap_remove(pmap_t pmap, vaddr_t va);

/*
 * Activate a pmap
 */
void pmap_activate(pmap_t pmap);

/*
 * Invalidate page
 */
void pmap_invlpg(vaddr_t va);

void vm_init(charon_t charon);

#endif
