/**
 * @file vmem.h
 * @brief Implementation of the VMem resource allocator as described in https://www.usenix.org/legacy/event/usenix01/full_papers/bonwick/bonwick.pdf
*/

#ifndef _VMEM_H
#define _VMEM_H
#include <sys/queue.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Directs vmem to use the smallest
free segment that can satisfy the allocation. This
policy tends to minimize fragmentation of very
small, precious resources (cited from paper) */
#define VM_BESTFIT (1 << 0)

/** Directs vmem to provide a
good approximation to best−fit in guaranteed
constant time. This is the default allocation policy. (cited from paper) */
#define VM_INSTANTFIT (1 << 1)

/** Directs vmem to use the next free
segment after the one previously allocated. This is
useful for things like process IDs, where we want
to cycle through all the IDs before reusing them. (cited from paper) */
#define VM_NEXTFIT (1 << 2)

#define VM_SLEEP (1 << 3)
#define VM_NOSLEEP (1 << 4)

/** Used to eliminate cyclic dependencies when refilling the segment freelist:
   We need to allocate new segments but to allocate new segments, we need to refill the list, this flag ensures that no refilling occurs. */
#define VM_BOOTSTRAP (1 << 5)

#define VMEM_ERR_NO_MEM 1

struct vmem;

/** vmem_t allows one arena to import its resources from
another. vmem_create() specifies the source arena,
and the functions to allocate and free from that source. The arena imports new spans as needed, and gives
them back when all their segments have been freed. (cited from paper) These types describe those functions.
 */
typedef void *vmem_alloc_t(struct vmem *vmem, size_t size, int flags);
typedef void vmem_free_t(struct vmem *vmem, void *addr, size_t size);

/* We can't use boundary tags because the resource we're managing is not necessarily memory.
   To counter this, we can use *external boundary tags*. For each segment in the arena
   we allocate a boundary tag to manage it. */

/** sizeof(void *) * CHAR_BIT (8) freelists provides us with a freelist for every power-of-2 length that can fit within the host's virtual address space (64 bit) */
#define FREELISTS_N sizeof(void *) * CHAR_BIT
#define HASHTABLES_N 16

/** Describes a Vmem segment */
typedef struct vmem_segment {
    enum { SEGMENT_ALLOCATED, SEGMENT_FREE, SEGMENT_SPAN } type;

    bool imported; /**< Non-zero if imported */

    uintptr_t base; /**< base address of the segment */
    uintptr_t size; /**< size of the segment */

    /* clang-format off */
  TAILQ_ENTRY(vmem_segment) segqueue; /**< Points to vmem_t::segqueue */
  LIST_ENTRY(vmem_segment) seglist; /**< If free, points to vmem_t::freelist, if allocated, points to vmem_t::hashtable, else vmem_t::spanlist */
    /* clang-format on */

} vmem_segment_t;

typedef LIST_HEAD(vmem_seglist, vmem_segment) vmem_seglist_t;
typedef TAILQ_HEAD(vmem_segqueue, vmem_segment) vmem_segqueue_t;

/* Statistics about a vmem_t arena, NOTE: this isn't described in the original paper and was added by me. Inspired by Illumos and Solaris'vmem_kstat_t */
typedef struct {
    size_t in_use; /* Memory in use */
    size_t import; /* Imported memory */
    size_t total; /* Total memory in the area */
    size_t alloc; /* Number of allocations */
    size_t free; /* Number of frees */
} vmem_stat_t;

/** Description of an arena, a collection of resources. An arena is simply a set of integers. */
typedef struct vmem {
    char name[64]; /**< Descriptive name for debugging purposes */
    void *base; /**< Start of initial span */
    size_t size; /**< Size of initial span */
    size_t quantum; /**< Unit of currency */
    vmem_alloc_t *alloc; /**< Import alloc function */
    vmem_free_t *free; /**< Import free function */
    struct vmem *source; /**< Import arena */
    size_t qcache_max; /**< Maximum size to cache */
    int vmflag; /**< VM_SLEEP or VM_NOSLEEP */

    vmem_segqueue_t segqueue; /**< Segment queue */
    vmem_seglist_t freelist
            [FREELISTS_N]; /**< Power of two freelists. Freelists[n] contains all free segments whose sizes are in the range [2^n, 2^n+1]  */
    vmem_seglist_t hashtable[HASHTABLES_N]; /**< Allocated segments */
    vmem_seglist_t spanlist; /**< Span marker segments */

    vmem_stat_t stat; /**< Statistical data about the arena */
} vmem_t;

/**
 * Initializes a vmem arena
 * @param vmem The arena to initialize
 * @param name Descriptive name for debugging purposes
 * @param base Start of initial span
 * @param size Size of initial span
 * @param quantum Unit of currency
 * @param afunc Imported alloc function
 * @param ffunc Imported free function
 * @param source Imported arena
 * @param qcache_max Maximum size to cache
 * @param vmflag VM_SLEEP or VM_NOSLEEP
 * @returns 0 on success
 */
int vmem_init(vmem_t *vmem, char *name, void *base, size_t size, size_t quantum,
              vmem_alloc_t *afunc, vmem_free_t *ffunc, vmem_t *source,
              size_t qcache_max, int vmflag);

/**
 * Destroys an area
 * @param vmp Arena to destroy
*/
void vmem_destroy(vmem_t *vmp);

/**
 * Allocates a resource from an arena
 * @param vmp The arena to allocate from
 * @param size Size of the allocation
 * @param vmflag may specify an allocation policy (VM_BESTFIT, VM_INSTANTFIT, or VM_NEXTFIT), default is VM_INSTANTFIT.
 * @returns Allocated resource
*/
void *vmem_alloc(vmem_t *vmp, size_t size, int vmflag);

/**
 * Frees a resource from an arena
 * @param vmp The arena to free from
 * @param addr The address to free
 * @param size The size to free
 */
void vmem_free(vmem_t *vmp, void *addr, size_t size);

/*
Allocates size bytes at offset phase from an align boundary such that the resulting segment
[addr, addr + size) is a subset of [minaddr, maxaddr) that does not straddle a nocross−
aligned boundary. vmflag is as above. One performance caveat: if either minaddr or maxaddr is
non−NULL, vmem may not be able to satisfy the allocation in constant time. If allocations within a
given [minaddr, maxaddr) range are common it is more efficient to declare that range to be its own
arena and use unconstrained allocations on the new arena (cited from paper).
*/
void *vmem_xalloc(vmem_t *vmp, size_t size, size_t align, size_t phase,
                  size_t nocross, void *minaddr, void *maxaddr, int vmflag);

/*
  Frees size bytes at addr, where addr was a constrained allocation. vmem_xfree() must be used if
the original allocation was a vmem_xalloc() because both routines bypass the quantum caches. (Cited from paper)
*/
void vmem_xfree(vmem_t *vmp, void *addr, size_t size);

/**
 * Adds a span to an arena
 * @param vmp The arena to add a span to
 * @param addr The start of the span
 * @param size The size of the span
 * @param vmflag VM_NOSLEEP or VM_SLEEP
 * @returns NULL on failure, addr on success
 */
void *vmem_add(vmem_t *vmp, void *addr, size_t size, int vmflag);

/**
 * Dumps an arena
 * @param vmp The arena to dump
 */
void vmem_dump(vmem_t *vmp);

/**
 * Early bootstrap
 */
void vmem_bootstrap(void);

#endif
