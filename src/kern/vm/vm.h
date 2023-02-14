/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file vm.h
 * @brief Virtual memory subsystem
 *
 * The whole VM folder describes and implements the virtual memory subsystem used by Gaia.
 * Most of the design decisions were derived from NetBSD's excellent UVM.
 */

/* TODO: add support for pagers */

#ifndef SRC_KERN_VM_VM_H_
#define SRC_KERN_VM_VM_H_
#include <machdep/vm.h>
#include <kern/vm/phys.h>
#include <kern/vm/vmem.h>
#include <kern/sync.h>

typedef enum {
    VM_PROT_READ = (1 << 0),
    VM_PROT_WRITE = (1 << 1),
    VM_PROT_EXECUTE = (1 << 2),
} vm_prot_t;

typedef uint8_t vattr_t;

typedef enum {
    PMAP_NONE,
    PMAP_HUGE,
    PMAP_LARGE,
} pmap_flags_t;

typedef struct pmap pmap_t;

struct vm_map;

void pmap_init(void);

pmap_t pmap_create(void);

void pmap_destroy(pmap_t pmap);

/*
 * Create a mapping in physical map pmap for the physical address pa at the virtual address va with protection prot
 */
int pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
               pmap_flags_t flags);

/*
 * Remove mapping.
 */
void pmap_remove(pmap_t pmap, vaddr_t va);

/*
 * Activate a pmap
 */
void pmap_activate(pmap_t pmap);

/*
 * Invalidate page
 */
void pmap_invlpg(vaddr_t va);

void vm_init(charon_t charon);

/**
 * Describes a physical page, vm_pages are allocated at boot
 */
typedef struct vm_page {
    bool unused;
    LIST_ENTRY(vm_page) list;
} vm_page_t;

/**
 * Describes a pager-backed memory object: something that can be mapped and paged
 *
 * UVM's two-level mapping scheme means we can avoid chaining multiple objects together in order to achieve CoW optimizations.
 */
typedef struct {
    spinlock_t lock; /**< Object lock */
    LIST_HEAD(, vm_page) pages; /**< List of pages */
    size_t npages; /**< Number of pages */
    int refs; /**< Reference count */
} vm_object_t;

/**
 * Reference to an amap
 */
typedef struct {
    struct vm_amap *amap; /**< The Amap */
    size_t slot_offset; /**< The slot into the amap */
} vm_aref_t;

/**
 * Describes anonymous memory
 */
typedef struct vm_anon {
    spinlock_t lock;
    int ref; /**< Number of amaps referencing it */
    bool resident : 1; /**< Whether we are currently in memory */

    union {
        paddr_t physpage; /**< If resident, this points to the physical page */
        void *swap_data; /**< Swap data */
    } u;
} vm_anon_t;

/**
 * Describes a memory map entry
 */
typedef struct vm_map_entry {
    LIST_ENTRY(vm_map_entry) map_link; /**< Links into vm_map_t::entries */

    vaddr_t start; /**< Starting virtual address */
    vaddr_t end; /**< End virtual address */
    size_t offset; /**< Offset into the object */

    vm_object_t *obj; /**< Backing object */
} vm_map_entry_t;

/**
 * Describes a virtual memory map
 */
typedef struct vm_map {
    pmap_t pmap; /**< Physical map related to the map */
    vmem_t vmem; /**< Backing vmem arena */
    LIST_HEAD(, vm_map_entry) entries; /**< Doubly linked-list of entries */
} vm_map_t;

extern vm_map_t vm_kmap;

int vm_map_obj(vm_map_t *map, vm_object_t obj, vaddr_t *vaddrp, size_t size,
               size_t offset);

#endif
