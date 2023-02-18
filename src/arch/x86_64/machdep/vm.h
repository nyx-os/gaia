/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_MACHDEP_VM_H_
#define SRC_ARCH_X86_64_MACHDEP_VM_H_
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define HHDM_BASE 0xffff800000000000
#define KERN_BASE 0xffffffff80000000

#define P2V(addr) (((paddr_t)(addr)) + HHDM_BASE)
#define V2P(addr) (((vaddr_t)(addr)) - HHDM_BASE)

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

struct pmap {
    paddr_t pml4;
};

#endif
