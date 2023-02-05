#ifndef _KMEM_H
#define _KMEM_H

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#define KMEM_DEFAULT_ALIGN 8
#define KMEM_SMALL_SLAB_SIZE (PAGE_SIZE / 8)
#define KMEM_OBJECTS_PER_LARGE_SLAB 16

struct kmem_slab;
struct kmem_bufctl;
struct kmem_cache;

typedef void KmemCacheFunc(void *, size_t);

typedef struct kmem_cache
{
    char name[32];
    size_t size;
    int align;
    size_t slab_size; /* Size of a slab */
    KmemCacheFunc *constructor;
    KmemCacheFunc *destructor;

    struct kmem_slab *first_free;

    /* Freelist of slabs */
    TAILQ_HEAD(, kmem_slab) slabs;

    /* TODO: use an actual hashtable */
    SLIST_HEAD(, kmem_bufctl) hashtable;
} KmemCache;

typedef struct kmem_bufctl
{
    /* For small slabs, smaller than 1/8 of a page, this links into the freelist. For big slabs, this links into the kmem_cache::hashtable */
    SLIST_ENTRY(kmem_bufctl) list;
    uintptr_t address;
    struct kmem_slab *slab;
} KmemBufCtl;

typedef struct kmem_slab
{

    KmemCache *cache;                         /* Controlling cache */
    void *base;                               /* Base of allocated memory */
    TAILQ_ENTRY(kmem_slab) slablist;          /* Linkage into KmemCache::slabs */
    SLIST_HEAD(buflist, kmem_bufctl) buflist; /* Buffer freelist */

    size_t ref_count;    /* Slab reference count (how many chunks allocated) */
    size_t buffer_count; /* Number of buffers */
    size_t nfree;

    void *data;
} KmemSlab;

void kmem_cache_init(KmemCache *cache, char *name, size_t size, int align,
                     KmemCacheFunc *constructor, KmemCacheFunc *destructor);

void *kmem_cache_alloc(KmemCache *cache, int flags);
void kmem_cache_free(KmemCache *cache, void *ptr);
void kmem_cache_destroy(KmemCache *cache);

void kmem_bootstrap(void);

void kmem_cache_dump(KmemCache *cache);

void *kmem_alloc(size_t size);
void kmem_free(void *ptr, size_t size);

void *kmem_realloc(void *ptr, size_t origsize, size_t newsize);

#endif
