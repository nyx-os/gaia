/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include "gaia/spinlock.h"
#include <gaia/pmm.h>
#include <limine.h>
#include <paging.h>

#define PML_ENTRY(addr, offset) (size_t)(addr & ((uintptr_t)0x1ff << offset)) >> offset;

static Pagemap kernel_pagemap = {0};

static volatile struct limine_kernel_address_request kaddr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0};

extern uintptr_t text_start_addr, text_end_addr;
extern uintptr_t rodata_start_addr, rodata_end_addr;
extern uintptr_t data_start_addr, data_end_addr;

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

    // Map the first 4GB+ of memory to the higher half.
    for (size_t i = PAGE_SIZE; i < MAX(GIB(4), pmm_get_total_page_count() * PAGE_SIZE); i += PAGE_SIZE)
    {
        paging_map_page(&kernel_pagemap, i + MMAP_IO_BASE, i, PTE_PRESENT | PTE_WRITABLE, true);
    }

    // Map the kernel
    uintptr_t text_start = ALIGN_DOWN(text_start_addr, PAGE_SIZE);
    uintptr_t text_end = ALIGN_UP(text_end_addr, PAGE_SIZE);
    uintptr_t rodata_start = ALIGN_DOWN(rodata_start_addr, PAGE_SIZE);
    uintptr_t rodata_end = ALIGN_UP(rodata_end_addr, PAGE_SIZE);
    uintptr_t data_start = ALIGN_DOWN(data_start_addr, PAGE_SIZE);
    uintptr_t data_end = ALIGN_UP(data_end_addr, PAGE_SIZE);

    struct limine_kernel_address_response *kaddr = kaddr_request.response;

    // Text is readable and executable, writing is not allowed.
    for (uintptr_t i = text_start; i < text_end; i += PAGE_SIZE)
    {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        paging_map_page(&kernel_pagemap, i, phys, PTE_PRESENT, true);
    }

    // Read-only data is readable and NOT executable, writing is not allowed.
    for (uintptr_t i = rodata_start; i < rodata_end; i += PAGE_SIZE)
    {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        paging_map_page(&kernel_pagemap, i, phys, PTE_PRESENT | PTE_NOT_EXECUTABLE, true);
    }

    // Data is readable and writable, but is NOT executable.
    for (uintptr_t i = data_start; i < data_end; i += PAGE_SIZE)
    {
        uintptr_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        paging_map_page(&kernel_pagemap, i, phys, PTE_PRESENT | PTE_WRITABLE | PTE_NOT_EXECUTABLE, true);
    }
}

void paging_map_page(Pagemap *pagemap, uintptr_t vaddr, uintptr_t paddr, uint64_t flags, bool huge)
{
    lock_acquire(&pagemap->lock);

    size_t level4 = PML_ENTRY(vaddr, 39);
    size_t level3 = PML_ENTRY(vaddr, 30);
    size_t level2 = PML_ENTRY(vaddr, 21);
    size_t level1 = PML_ENTRY(vaddr, 12);

    uint64_t *pml3 = get_next_level((void *)host_phys_to_virt((uintptr_t)pagemap->pml4), level4, flags, true);
    uint64_t *pml2 = get_next_level(pml3, level3, flags, true);
    uint64_t *pml1 = get_next_level(pml2, level2, flags, true);

    if (huge)
    {
        pml2[level1] = paddr | flags | PTE_HUGE;
    }
    else
    {
        pml1[level1] = paddr | flags;
    }

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
