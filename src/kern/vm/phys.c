/* SPDX-License-Identifier: BSD-2-Clause */
#include "phys.h"
#include <sys/queue.h>

typedef struct phys_region {
    size_t size;
    SLIST_ENTRY(phys_region) entry;
} phys_region_t;

static SLIST_HEAD(freelist_head, phys_region) freelist_head;
static size_t total_page_count = 0;
static size_t used_pages = 0;

void phys_init(charon_t charon)
{
    SLIST_INIT(&freelist_head);

    for (size_t i = 0; i < charon.memory_map.count; i++) {
        charon_mmap_entry_t entry = charon.memory_map.entries[i];

        if (entry.type != MMAP_RESERVED)
            total_page_count += (entry.size / PAGE_SIZE);

        if (entry.type == MMAP_FREE) {
            phys_region_t *region = (phys_region_t *)P2V(entry.base);
            region->size = entry.size;

            SLIST_INSERT_HEAD(&freelist_head, region, entry);
        }
    }
}

paddr_t phys_alloc(void)
{
    if (SLIST_EMPTY(&freelist_head)) {
        panic("out of physical memory");
    }

    phys_region_t *region = SLIST_FIRST(&freelist_head);

    region->size -= PAGE_SIZE;

    if (region->size == 0) {
        SLIST_REMOVE_HEAD(&freelist_head, entry);
    }

    used_pages++;

    return V2P(region) + region->size;
}

void phys_free(paddr_t addr)
{
    phys_region_t *region = (void *)P2V(addr);

    region->size = PAGE_SIZE;

    SLIST_INSERT_HEAD(&freelist_head, region, entry);
    used_pages--;
}

paddr_t phys_allocz(void)
{
    paddr_t ret = phys_alloc();

    memset((void *)P2V(ret), 0, PAGE_SIZE);

    return ret;
}

size_t phys_used_pages(void)
{
    return used_pages;
}

size_t phys_total_pages(void)
{
    return total_page_count;
}
