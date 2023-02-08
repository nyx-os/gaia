/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file kmem.h
 * @brief Implementation of the object-caching KMem Slab allocator from Solaris
 */
#ifndef SRC_KERN_VM_KMEM_H_
#define SRC_KERN_VM_KMEM_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#define KMEM_DEFAULT_ALIGN 8
#define KMEM_SMALL_SLAB_SIZE (PAGE_SIZE / 8)
#define KMEM_OBJECTS_PER_LARGE_SLAB 16

struct kmem_slab;
struct kmem_bufctl;
struct kmem_cache;

typedef void kmem_cache_fn_t(void *, size_t);

typedef struct kmem_cache {
    char name[32]; /**< Descriptive name for debugging purposes */
    size_t size; /**< Size of the cache's allocations */
    int align; /**< Required alignment for the cache's allocations */
    size_t slab_size; /**< Size of a slab */
    kmem_cache_fn_t *constructor; /**< Constructor function for objects */
    kmem_cache_fn_t *destructor; /**< Destructor function for objects */

    TAILQ_HEAD(, kmem_slab) slabs; /**< Freelist of slabs */

    /* TODO: use an actual hashtable */
    SLIST_HEAD(, kmem_bufctl) hashtable; /**< Hashtables for keeping bufctls */
} kmem_cache_t;

typedef struct kmem_bufctl {
    SLIST_ENTRY(kmem_bufctl)
    list; /**< For small slabs, smaller than 1/8 of a page, this links into the freelist. For big slabs, this links into the kmem_cache::hashtable */
    uintptr_t address; /**< Address of the bufctl */
    struct kmem_slab *slab; /**< Back pointer to slab */
} kmem_bufctl_t;

typedef struct kmem_slab {
    kmem_cache_t *cache; /**< Controlling cache */
    void *base; /**< Base of allocated memory */
    TAILQ_ENTRY(kmem_slab) slablist; /**< Linkage into KmemCache::slabs */
    SLIST_HEAD(buflist, kmem_bufctl) buflist; /**< Buffer freelist */

    size_t ref_count; /**< Slab reference count (how many chunks allocated) */
    size_t buffer_count; /**< Number of buffers */
    size_t nfree; /**< Number of free objects in the slab */

    void *data; /**< Pointer to the slab's data */
} kmem_slab_t;

/**
 * Initialize a kmem cache
 * @param cache The cache to initialize
 * @param name Descriptive name for debugging purposes
 * @param size Size of allocations
 * @param align Required alignment for allocations, if 0 then KMEM_DEFAULT_ALIGN is used
 * @param constructor Constructor for objects
 * @param destructor Destructor for objects
 */
void kmem_cache_init(kmem_cache_t *cache, char *name, size_t size, int align,
                     kmem_cache_fn_t *constructor, kmem_cache_fn_t *destructor);

/**
 * Allocates memory from a kmem cache
 * @param cache The cache to allocate from
 * @param flags ignored
 * @return Pointer to allocated memory
 */
void *kmem_cache_alloc(kmem_cache_t *cache, int flags);

/**
 * Frees memory from a kmem cache
 * @param cache The cache to free from
 * @param ptr The pointer to free
 */
void kmem_cache_free(kmem_cache_t *cache, void *ptr);

/**
 * Destroys a kmem cache
 * @param cache The cache to destroy
 */
void kmem_cache_destroy(kmem_cache_t *cache);

/**
 * Bootstraps kmem
 */
void kmem_bootstrap(void);

/**
 * Dumps a kmem cache
 * @param cache The cache to dump.
 */
void kmem_cache_dump(kmem_cache_t *cache);

/**
 * Allocates memory
 * @param size Bytes to allocate
 * @return Pointer to allocated memory
 */
void *kmem_alloc(size_t size);

/**
 * Frees memory previously allocated with kmem_alloc
 * @param ptr The pointer to free
 * @param size The size to free
 */
void kmem_free(void *ptr, size_t size);

/**
 * Grows an allocation
 * @param ptr The pointer to grow, if NULL then it will be allocated
 * @param origsize The original size of the allocated memory
 * @param newsize The size to grow to
 * @return Pointer to the reallocated memory
 */
void *kmem_realloc(void *ptr, size_t origsize, size_t newsize);

#endif
