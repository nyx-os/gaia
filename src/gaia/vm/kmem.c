#include "kmem.h"
#include <gaia/debug.h>
#include <gaia/pmm.h>
#include <gaia/queue.h>
#include <gaia/vm/vm_kernel.h>
#include <stdbool.h>
#include <stdc-shim/string.h>

#define kmem_printf(...)

#define KERNEL

#ifndef KERNEL
#    include <assert.h>
#    include <malloc.h>
#    include <stdlib.h>
#    include <sys/mman.h>
#    include <unistd.h>
#    define PAGE_SIZE 4096
#    define ALIGN_UP(addr, align) (((addr) + align - 1) & ~(align - 1))
#    define ALIGN_DOWN(addr, align) ((((uintptr_t)addr)) & ~(align - 1))

#    define kmem_printf printf

static void *
vm_kernel_alloc(int npages)
{
    return mmap(NULL, npages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static void vm_kernel_free(void *ptr, int npages)
{
    munmap(ptr, npages * PAGE_SIZE);
}

#endif

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define APPLY_SLAB_SIZES(X) \
    X(8, kmem_8)            \
    X(16, kmem_16)          \
    X(24, kmem_24)          \
    X(32, kmem_32)          \
    X(40, kmem_40)          \
    X(48, kmem_48)          \
    X(56, kmem_56)          \
    X(64, kmem_64)          \
    X(80, kmem_80)          \
    X(96, kmem_96)          \
    X(112, kmem_112)        \
    X(128, kmem_128)        \
    X(160, kmem_160)        \
    X(192, kmem_192)        \
    X(224, kmem_224)        \
    X(256, kmem_256)        \
    X(320, kmem_320)        \
    X(384, kmem_384)        \
    X(448, kmem_448)        \
    X(512, kmem_512)        \
    X(640, kmem_640)        \
    X(768, kmem_768)        \
    X(896, kmem_896)        \
    X(1024, kmem_1024)      \
    X(1280, kmem_1280)      \
    X(1536, kmem_1536)      \
    X(1792, kmem_1792)      \
    X(2048, kmem_2048)      \
    X(2560, kmem_2560)      \
    X(3072, kmem_3072)      \
    X(3584, kmem_3584)      \
    X(4096, kmem_4096)

#define KMEM_MAXBUF 4096
#define KMEM_ALIGN_SHIFT 3 /* log2(8) */

#define SMALL_SLAB_HEADER(base) ((uintptr_t)base + PAGE_SIZE - sizeof(KmemSlab))

#define DEFINE_CACHE(SIZE, NAME) static KmemCache NAME;
APPLY_SLAB_SIZES(DEFINE_CACHE)
#undef DEFINE_CACHE

#define REF_CACHE(SIZE, NAME) &NAME,
static KmemCache *caches[] = {
    APPLY_SLAB_SIZES(REF_CACHE)};

static KmemCache slab_cache;
static KmemCache bufctl_cache;

static size_t cache_slab_size(KmemCache *cache)
{
    if (cache->size <= KMEM_SMALL_SLAB_SIZE)
    {
        return PAGE_SIZE;
    }
    else
    {
        return ALIGN_UP(cache->size * KMEM_OBJECTS_PER_LARGE_SLAB, PAGE_SIZE);
    }
    return 0;
}

static size_t cache_slab_cap(KmemCache *cache)
{
    size_t slab_size = cache_slab_size(cache);

    if (cache->size <= KMEM_SMALL_SLAB_SIZE)
    {
        /* Since the slab data is put at the end of the page, we need to do the following calculation:
    get the slab size, in this case, 1 page, substract the size of the slab header from it and divide it into equally-sized chunks */
        return (slab_size - sizeof(KmemSlab)) / cache->size;
    }

    return slab_size / cache->size;
}

static KmemSlab *kmem_small_slab_create(KmemCache *cache)
{
    size_t i;
    KmemSlab *ret;
    void *base;
    size_t cap = cache_slab_cap(cache);
    KmemBufCtl *buf = NULL;

    base = vm_kernel_alloc(cache_slab_size(cache) / PAGE_SIZE, false);

    assert(base != NULL);

    ret = (KmemSlab *)SMALL_SLAB_HEADER(base);

    TAILQ_INSERT_HEAD(&cache->slabs, ret, slablist);

    SLIST_INSERT_HEAD(&ret->buflist, (KmemBufCtl *)base, list);

    ret->base = base;
    ret->nfree = cap;
    ret->cache = cache;

    for (i = 0; i < cap; i++)
    {
        buf = (KmemBufCtl *)((uintptr_t)base + i * cache->size);

        SLIST_NEXT(buf, list) = (KmemBufCtl *)((uintptr_t)base + (i + 1) * cache->size);

        if (cache->constructor != NULL)
        {
            cache->constructor((uint8_t *)base + i * cache->size, cache->size);
        }
    }

    SLIST_NEXT(buf, list) = NULL;

    return ret;
}

static KmemSlab *kmem_large_slab_create(KmemCache *cache)
{
    KmemSlab *new_slab = NULL;
    KmemBufCtl *buf = NULL, *prev = NULL;
    size_t cap = cache_slab_cap(cache);
    size_t size = cache_slab_size(cache);
    size_t i;

    new_slab = kmem_cache_alloc(&slab_cache, 0);

    TAILQ_INSERT_HEAD(&cache->slabs, new_slab, slablist);
    new_slab->nfree = cap;
    new_slab->cache = cache;
    new_slab->data = vm_kernel_alloc(size / PAGE_SIZE, false);

    for (i = 0; i < cap; i++)
    {
        buf = kmem_cache_alloc(&bufctl_cache, 0);
        buf->slab = new_slab;
        buf->address = (uintptr_t)new_slab->data + cache->size * i;

        if (prev)
        {
            SLIST_NEXT(prev, list) = buf;
        }
        else
        {
            SLIST_INSERT_HEAD(&new_slab->buflist, buf, list);
        }

        if (cache->constructor != NULL)
        {
            cache->constructor(buf, cache->size);
        }

        prev = buf;
    }

    SLIST_NEXT(buf, list) = NULL;

    return new_slab;
}

static KmemSlab *kmem_slab_create(KmemCache *cache)
{
    if (cache->size <= KMEM_SMALL_SLAB_SIZE)
    {
        return kmem_small_slab_create(cache);
    }

    return kmem_large_slab_create(cache);
}

static void kmem_slab_destroy(KmemSlab *slab)
{
    size_t slabpages = cache_slab_size(slab->cache) / PAGE_SIZE;
    size_t i;

    if (slab->cache->destructor)
    {
        for (i = 0; i < cache_slab_cap(slab->cache); i++)
        {
            slab->cache->destructor((uint8_t *)slab->base + i * slab->cache->size, slab->cache->size);
        }
    }

    vm_kernel_free(slab->base, slabpages);
}

void kmem_bootstrap(void)
{
    kmem_cache_init(&slab_cache, "Slab cache", sizeof(KmemSlab) + sizeof(void *), 0, NULL, NULL);
    kmem_cache_init(&bufctl_cache, "Bufctl cache", sizeof(KmemBufCtl), 0, NULL, NULL);

#define CACHE_INIT(SIZE, NAME) kmem_cache_init(&NAME, #NAME, SIZE, 0, NULL, NULL);
    APPLY_SLAB_SIZES(CACHE_INIT);
#undef CACHE_INIT
}

void kmem_cache_init(KmemCache *cache, char *name, size_t size, int align,
                     KmemCacheFunc *constructor, KmemCacheFunc *destructor)
{
    if (align == 0)
        align = KMEM_DEFAULT_ALIGN;

    strncpy(cache->name, name, strlen(name));
    cache->size = size;
    cache->align = align;
    cache->constructor = constructor;
    cache->destructor = destructor;

    SLIST_INIT(&cache->hashtable);
    TAILQ_INIT(&cache->slabs);

    kmem_slab_create(cache);
}

void *kmem_cache_alloc(KmemCache *cache, int flags)
{
    void *ret;
    KmemSlab *sp;
    KmemBufCtl *first_free_buf, *next_buf;

    assert(flags == 0 && "Not supported yet");

    assert(cache != NULL);

    sp = TAILQ_FIRST(&cache->slabs);

    if (!sp || sp->nfree == 0)
    {
        sp = kmem_slab_create(cache);
    }

    first_free_buf = SLIST_FIRST(&sp->buflist);

    assert(first_free_buf != NULL);

    next_buf = SLIST_NEXT(first_free_buf, list);

    /* Slab is now empty */
    if (!next_buf)
    {
        TAILQ_REMOVE(&cache->slabs, sp, slablist);
        TAILQ_INSERT_TAIL(&cache->slabs, sp, slablist);

        sp->buflist.slh_first = NULL;
    }

    else
    {
        SLIST_FIRST(&sp->buflist) = next_buf;
    }

    sp->nfree--;
    sp->ref_count++;

    if (cache->size <= KMEM_SMALL_SLAB_SIZE)
    {
        ret = (void *)first_free_buf;
    }
    else
    {
        SLIST_INSERT_HEAD(&cache->hashtable, first_free_buf, list);
        ret = (void *)first_free_buf->address;
    }

    return ret;
}

void kmem_cache_free(KmemCache *cache, void *ptr)
{
    KmemSlab *slab;
    KmemBufCtl *new_free;

    if (cache->size <= KMEM_SMALL_SLAB_SIZE)
    {
        slab = (KmemSlab *)SMALL_SLAB_HEADER(ALIGN_DOWN((uintptr_t)ptr, 4096));
        new_free = (KmemBufCtl *)ptr;
    }

    else
    {
        KmemBufCtl *buf;

        new_free = NULL;

        /* Find bufctl for pointer */
        SLIST_FOREACH(buf, &cache->hashtable, list)
        {
            if (buf->address == (uintptr_t)ptr)
            {
                new_free = buf;
                break;
            }
        }

        assert(new_free && "invalid pointer");

        slab = new_free->slab;
    }

    assert(slab->ref_count >= 1);

    /* TODO: if ref_count == 0, destroy slab */

    SLIST_NEXT(new_free, list) = slab->buflist.slh_first;
    SLIST_INSERT_HEAD(&slab->buflist, new_free, list);
}

void kmem_cache_destroy(KmemCache *cache)
{
    KmemSlab *slab;

    TAILQ_FOREACH(slab, &cache->slabs, slablist)
    {
        kmem_slab_destroy(slab);
    }
}

void kmem_cache_dump(KmemCache *cache)
{
    size_t slab_num = 0;
    size_t total_free = 0;
    size_t cap = cache_slab_cap(cache);
    size_t used_objs = 0;

    KmemSlab *slab;

    (void)used_objs;

    kmem_printf("Cache \"%s\":\n", cache->name);
    TAILQ_FOREACH(slab, &cache->slabs, slablist)
    {
        slab_num++;
        total_free += slab->nfree;
    }

    used_objs = (cap * slab_num) - total_free;

    kmem_printf("  - Slab count: %ld\n", slab_num);
    kmem_printf("  - Used memory: %ld bytes (%ld objects) / %ld bytes (%ld objects)\n",
                used_objs * cache->size, used_objs, cap * slab_num * cache->size, cap * slab_num);
}

static inline int
cachenum(size_t size)
{
    if (size <= 64)
        return ALIGN_UP(size, 8) / 8 - 1;
    else if (size <= 128)
        return ALIGN_UP(size - 64, 16) / 16 + 7;
    else if (size <= 256)
        return ALIGN_UP(size - 128, 32) / 32 + 11;
    else if (size <= 512)
        return ALIGN_UP(size - 256, 64) / 64 + 15;
    else if (size <= 1024)
        return ALIGN_UP(size - 512, 128) / 128 + 19;
    else if (size <= 2048)
        return ALIGN_UP(size - 1024, 256) / 256 + 23;
    else if (size <= 4096)
        return ALIGN_UP(size - 2048, 512) / 512 + 27;
    else
        return -1;
}

void *kmem_alloc(size_t size)
{
    int index = cachenum(size);

    if (index == -1)
    {
        return vm_kernel_alloc(ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE, false);
    }

    return kmem_cache_alloc(caches[index], 0);
}

void kmem_free(void *ptr, size_t size)
{
    int index = cachenum(size);

    if (index == -1)
    {
        vm_kernel_free(ptr, ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE);
        return;
    }

    kmem_cache_free(caches[index], ptr);
}

void *kmem_realloc(void *ptr, size_t origsize, size_t newsize)
{
    void *newptr = ptr;

    newptr = kmem_alloc(newsize);

    if (ptr != NULL)
    {
        assert(origsize > 0);
        assert(newsize > origsize);

        memcpy(newptr, ptr, origsize);
        kmem_free(ptr, origsize);
    }

    return newptr;
}
