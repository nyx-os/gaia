/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file phys.h
 * @brief Physical page management
 */
#ifndef SRC_KERN_VM_PHYS_H_
#define SRC_KERN_VM_PHYS_H_
#include <kern/charon.h>
#include <machdep/vm.h>

/**
 * Initializes the subsystem
 * @param charon Boot info
 */
void phys_init(charon_t charon);

/**
 * Allocates a physical page
 * @return Allocated physical page
 */
paddr_t phys_alloc(void);

/**
 * Allocates a physical page then zeroes its contents
 * @return Allocated physical page
 */
paddr_t phys_allocz(void);

/**
 * Frees a physical page
 * @param addr The page to free
 */
void phys_free(paddr_t addr);

/**
 * @return Used physical pages
 */
size_t phys_used_pages(void);

/**
 * @return Total usable physical pages
 */
size_t phys_usable_pages(void);

/**
 * @return Highest page in physical memory
 */
paddr_t phys_highest_page(void);
#endif
