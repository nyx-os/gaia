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

#define MMAP_ANONYMOUS (1 << 0)
#define MMAP_FIXED (1 << 1)
#define MMAP_SHARED (1 << 2)
#define MMAP_PRIVATE (1 << 3)

/* NOTE: this is specific to gaia */
#define MMAP_PHYS (1 << 4)

#define PROT_NONE (1 << 0)
#define PROT_READ (1 << 1)
#define PROT_WRITE (1 << 2)
#define PROT_EXEC (1 << 3)

#define MMAP_BUMP_BASE 0x100000000

typedef struct vmm_map_range
{
    uintptr_t address, phys;
    uint16_t prot, flags;
    size_t size, allocated_size;
    struct vmm_map_range *next;
} VmmMapRange;

typedef struct
{
    uintptr_t bump;
    VmmMapRange *ranges_head;
    Pagemap *pagemap;
} VmmMapSpace;

void vmm_space_init(VmmMapSpace *space);
void *vmm_mmap(VmmMapSpace *space, uint16_t prot, uint16_t flags, void *addr, void *phys, size_t size);
void vmm_munmap(VmmMapSpace *space, uintptr_t addr);
void vmm_write(VmmMapSpace *space, uintptr_t address, void *data, size_t count);
bool vmm_page_fault_handler(VmmMapSpace *space, uintptr_t faulting_address);

#endif
