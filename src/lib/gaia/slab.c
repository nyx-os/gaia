
/* Copyright 2022 mintsuki and contributors. */
/* From https://github.com/lyre-os/lyre/blob/trunk/kernel/mm/slab.c, modified for gaia */

#include "gaia/host.h"
#include <gaia/base.h>
#include <gaia/slab.h>
#include <gaia/spinlock.h>
#include <gaia/vm/vm_kernel.h>
#include <stdc-shim/string.h>

typedef struct
{
    Spinlock lock;
    size_t entry_size;
    size_t used;
    void **head;
} Slab;

typedef struct
{
    Slab *slab;
} SlabHeader;

static size_t slab_used_mem = 0;

static Slab slabs[SLAB_COUNT] = {0};

static Slab *find_slab(size_t size)
{
    for (size_t i = 0; i < sizeof(slabs) / sizeof(*slabs); i++)
    {
        if (slabs[i].entry_size >= size)
        {
            return &slabs[i];
        }
    }

    return NULL;
}

static void create_slab(Slab *slab, size_t size)
{
    slab->lock = 0;

    // Allocate the head of the slab
    slab->head = (void *)(vm_kernel_alloc(1, false));

    slab->entry_size = size;

    // initialize the slab's header
    SlabHeader *slab_header = (SlabHeader *)slab->head;

    slab_header->slab = slab;

    // Get the offset of the header then put the 'head' aka the first free block there.
    size_t header_offset = ALIGN_UP(sizeof(SlabHeader), size);
    slab->head = (void **)((uint8_t *)slab->head + header_offset);

    // Now, go through each slab and initialize them
    SlabHeader **headers = (SlabHeader **)(slab->head);

    // FIXME: Stop hardcoding page size
    size_t number_of_slabs = (4096 - header_offset) / size - 1;

    for (size_t i = 0; i < number_of_slabs; i++)
    {
        headers[i] = headers[(i + 1)];
    }

    headers[number_of_slabs] = NULL;
}

static void *allocate_from_slab(Slab *slab)
{
    lock_acquire(&slab->lock);

    if (!slab->head)
    {
        create_slab(slab, slab->entry_size);
    }

    void **old_head = slab->head;

    slab->head = *slab->head;

    slab_used_mem += slab->entry_size;
    slab->used += slab->entry_size;

    memset(old_head, 0, slab->entry_size);

    lock_release(&slab->lock);
    return old_head;
}

static void free_from_slab(Slab *slab, void *ptr)
{
    lock_acquire(&slab->lock);
    *(void **)ptr = slab->head;
    slab->head = (void *)ptr;
    slab_used_mem -= slab->entry_size;
    slab->used -= slab->entry_size;
    lock_release(&slab->lock);
}

void slab_init(void)
{
    create_slab(&slabs[0], 32);
    create_slab(&slabs[1], 64);
    create_slab(&slabs[2], 128);
    create_slab(&slabs[3], 256);
    create_slab(&slabs[4], 512);
    create_slab(&slabs[5], 1024);
    create_slab(&slabs[6], 2048);
}

void *slab_alloc(size_t size)
{
    Slab *slab = find_slab(size);
    if (!slab)
    {
        panic("Couldn't find slab for size %d", size);
    }
    return allocate_from_slab(slab);
}

void slab_free(void *ptr)
{
    assert(ptr != NULL);
    SlabHeader *slab_header = (SlabHeader *)((uintptr_t)ptr & ~0xfff);
    free_from_slab(slab_header->slab, ptr);
}

void *slab_realloc(void *ptr, size_t size)
{
    if (!ptr)
    {
        return slab_alloc(size);
    }

    SlabHeader *slab_header = (SlabHeader *)((uintptr_t)ptr & ~0xfff);
    Slab *slab = slab_header->slab;

    if (size > slab->entry_size)
    {
        void *new_ptr = slab_alloc(size);
        host_accelerated_copy(new_ptr, ptr, slab->entry_size);
        free_from_slab(slab, ptr);
        return new_ptr;
    }
    else
    {
        return ptr;
    }
}

size_t slab_used(void)
{
    return slab_used_mem;
}

void slab_dump(void)
{
    for (int i = 0; i < SLAB_COUNT; i++)
    {
        trace("slab %d (block size: %d): %d bytes allocated (%d objects)", i, slabs[i].entry_size, slabs[i].used, slabs[i].used / slabs[i].entry_size);
    }
}
