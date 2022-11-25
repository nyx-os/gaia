/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
   @file vmm.h
   @brief Virtual Memory Manager for userspace tasks
*/

#ifndef SRC_GAIA_VMM_H
#define SRC_GAIA_VMM_H
#include <gaia/base.h>

#define VM_MEM_DMA (1 << 0)

#define VM_PROT_NONE (1 << 0)
#define VM_PROT_READ (1 << 1)
#define VM_PROT_WRITE (1 << 2)
#define VM_PROT_EXEC (1 << 3)

#define VM_MAP_ANONYMOUS (1 << 0)
#define VM_MAP_FIXED (1 << 1)
#define VM_MAP_PHYS (1 << 2)


#define MMAP_BUMP_BASE 0x100000000

typedef struct PACKED
{
    size_t size;
    uintptr_t addr;
    uint16_t flags;
} VmCreateArgs;

typedef struct
{
    void *buf;
    size_t size;
} VmObject;

typedef struct PACKED
{
    VmObject *object;
    uint16_t protection;
    uintptr_t vaddr;
    uint16_t flags;
} VmMapArgs;

typedef struct vm_mapping
{
    VmObject object;
    size_t allocated_size;
    uintptr_t phys, address;
    uint16_t actual_protection;
    struct vm_mapping *next;
} VmMapping;

typedef struct
{
    uintptr_t bump;
    VmMapping *mappings;
    Pagemap *pagemap;
} VmmMapSpace;

void vmm_space_init(VmmMapSpace *space);
void vmm_write(VmmMapSpace *space, uintptr_t address, void *data, size_t count);
bool vmm_page_fault_handler(VmmMapSpace *space, uintptr_t faulting_address);

VmObject vm_create(VmCreateArgs args);
int vm_map(VmmMapSpace *space, VmMapArgs args);
void vmm_vm_unmap(VmmMapSpace *space, uintptr_t addr);

#endif
