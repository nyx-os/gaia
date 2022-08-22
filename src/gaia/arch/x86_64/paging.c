/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include "gaia/spinlock.h"
#include <gaia/pmm.h>
#include <paging.h>

#define PML_ENTRY(addr, offset) (size_t)(addr & ((uintptr_t)0x1ff << offset)) >> offset;

static Pagemap kernel_pagemap = {0};

static uint64_t *get_next_level(uint64_t *table, size_t index, size_t flags, bool allocate)
{

    // If the entry is already present, just return it.
    if (PTE_IS_PRESENT(table[index]))
    {
        return (uint64_t *)host_phys_to_virt(PTE_GET_ADDR(table[index]));
    }

    // If there's no entry in the page table and we don't want to allocate, we abort.
    if (!allocate)
    {
        return NULL;
    }

    // Otherwise, we allocate a new entry
    uintptr_t new_table = (uintptr_t)pmm_alloc();

    assert(new_table != 0);

    table[index] = new_table | flags;

    return (uint64_t *)host_phys_to_virt(new_table);
}

static void invlpg(void *addr)
{
    __asm__ volatile("invlpg (%0)"
                     :
                     : "r"(addr)
                     : "memory");
}

void paging_initialize()
{
    kernel_pagemap.pml4 = pmm_alloc_zero();
    kernel_pagemap.lock._lock = 0;

    assert(kernel_pagemap.pml4 != NULL);

    for (size_t i = 256; i < 512; i++)
    {
        assert(get_next_level(kernel_pagemap.pml4, i, PTE_PRESENT | PTE_WRITABLE | PTE_USER, true) != NULL);
    }

    // Map the first 4GB+ of memory to the higher half.
    for (size_t i = PAGE_SIZE; i < MAX(GIB(4), pmm_get_total_page_count() * PAGE_SIZE); i += PAGE_SIZE)
    {
        paging_map_page(&kernel_pagemap, i + MMAP_IO_BASE, i, PTE_PRESENT | PTE_WRITABLE);
    }
}

void paging_map_page(Pagemap *pagemap, uintptr_t vaddr, uintptr_t paddr, uint64_t flags)
{
    lock_acquire(&pagemap->lock);

    size_t level4 = PML_ENTRY(vaddr, 39);
    size_t level3 = PML_ENTRY(vaddr, 30);
    size_t level2 = PML_ENTRY(vaddr, 21);
    size_t level1 = PML_ENTRY(vaddr, 12);

    uint64_t *pml3 = get_next_level((void *)host_phys_to_virt((uintptr_t)pagemap->pml4), level4, flags, true);
    uint64_t *pml2 = get_next_level(pml3, level3, flags, true);
    uint64_t *pml1 = get_next_level(pml2, level2, flags, true);

    pml1[level1] = paddr | flags;

    lock_release(&pagemap->lock);
}

void paging_unmap_page(Pagemap *pagemap, uintptr_t vaddr)
{
    size_t level4 = PML_ENTRY(vaddr, 39);
    size_t level3 = PML_ENTRY(vaddr, 30);
    size_t level2 = PML_ENTRY(vaddr, 21);
    size_t level1 = PML_ENTRY(vaddr, 12);

    uint64_t *pml3 = get_next_level((void *)host_phys_to_virt((uintptr_t)pagemap->pml4), level4, 0, false);
    uint64_t *pml2 = get_next_level(pml3, level3, 0, false);
    uint64_t *pml1 = get_next_level(pml2, level2, 0, false);

    pml1[level1] = 0;

    invlpg((void *)vaddr);
}

void paging_load_pagemap(Pagemap *pagemap)
{
    __asm__ volatile("mov %0, %%cr3"
                     :
                     : "r"(pagemap->pml4)
                     : "memory");
}

Pagemap *paging_get_kernel_pagemap()
{
    return &kernel_pagemap;
}
