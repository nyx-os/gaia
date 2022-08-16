/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/charon.h"
#include "gaia/host.h"
#include "gaia/spinlock.h"
#include <gaia/base.h>
#include <gaia/pmm.h>
#include <stdc-shim/string.h>

typedef struct
{
    size_t size;
    uint64_t next;
} FreelistItem;

static Spinlock pmm_lock = {0};
static size_t total_page_count = 0;
static size_t free_pages = 0;
static size_t used_pages = 0;

static FreelistItem *freelist_head = NULL;

void pmm_init(Charon charon)
{
    CharonMemoryMap mmap = charon.memory_map;

    for (size_t i = 0; i < mmap.count; i++)
    {
        total_page_count += mmap.entries[i].size / PAGE_SIZE;

        log("mmap entry %p-%p, type=%d", mmap.entries[i].base, mmap.entries[i].base + mmap.entries[i].size, mmap.entries[i].type);

        if (mmap.entries[i].type == MMAP_FREE)
        {
            free_pages += mmap.entries[i].size / PAGE_SIZE;

            FreelistItem *new_item = (FreelistItem *)host_phys_to_virt(mmap.entries[i].base);

            new_item->next = (uintptr_t)freelist_head;

            new_item->size = mmap.entries[i].size;

            freelist_head = (FreelistItem *)mmap.entries[i].base;
        }
    }
}

static void *allocate_item(FreelistItem *item)
{
    // If the current item is now empty, remove it from the freelist.
    if (item->size == 0)
    {
        void *result = freelist_head;
        freelist_head = (FreelistItem *)item->next;
        return result;
    }
    else
    {
        void *result = (void *)((uintptr_t)freelist_head + item->size);
        return result;
    }
}

void *pmm_alloc()
{
    lock_acquire(&pmm_lock);

    if (!freelist_head)
    {
        lock_release(&pmm_lock);
        return NULL;
    }

    free_pages--;
    used_pages++;

    FreelistItem *item = (FreelistItem *)host_phys_to_virt((uintptr_t)freelist_head);
    item->size -= PAGE_SIZE;
    void *result = allocate_item(item);
    lock_release(&pmm_lock);
    return result;
}

void *pmm_alloc_zero()
{
    void *result = pmm_alloc();
    memset((void *)host_phys_to_virt((uintptr_t)result), 0, PAGE_SIZE);
    return result;
}

void pmm_free(void *ptr)
{

    if (!ptr)
        return;

    lock_acquire(&pmm_lock);

    free_pages++;
    used_pages--;

    FreelistItem *item = (FreelistItem *)host_phys_to_virt((uintptr_t)ptr);
    item->size = PAGE_SIZE;

    item->next = (uintptr_t)freelist_head;
    freelist_head = (FreelistItem *)ptr;

    lock_release(&pmm_lock);
}

void pmm_dump(void)
{
    FreelistItem *item = (FreelistItem *)host_phys_to_virt((uintptr_t)freelist_head);

    trace("Freelist dump:");

    while (host_virt_to_phys((uintptr_t)item))
    {
        trace("%p-%p", host_virt_to_phys((uintptr_t)item), host_virt_to_phys((uintptr_t)item) + item->size);
        item = (FreelistItem *)host_phys_to_virt(item->next);
    }
}

size_t pmm_get_total_page_count(void)
{
    return total_page_count;
}
