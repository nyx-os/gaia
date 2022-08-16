/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/** @file
 *  @brief Physical memory manager/Page frame allocator.
 */

#ifndef SRC_GAIA_PMM_H
#define SRC_GAIA_PMM_H
#include <gaia/base.h>
#include <gaia/charon.h>

// FIXME: we're assuming that the page size is 4096 bytes, while there are some architectures where this is not true.
#define PAGE_SIZE 4096
#define MMAP_IO_BASE ((uintptr_t)0xffff800000000000)
#define MMAP_KERNEL_BASE ((uintptr_t)0xffffffff80000000)

/** @brief Allocates one phyiscal page.
 *  @returns A pointer to the page.
 */
void *pmm_alloc(void);

/** @brief Allocates one physical page and sets it to zero.
 * @returns A pointer to the page.
 */
void *pmm_alloc_zero(void);

/** @brief Frees physical pages.
 *  @param ptr The page to free.
 */
void pmm_free(void *ptr);

/** @brief Initializes the pmm.
 *  @param charon Boot information.
 */
void pmm_init(Charon charon);

/** @brief Prints out debugging information for the pmm. */
void pmm_dump(void);

/** @brief Get the number of free pages.
 *  @returns The number of free pages.
 */
size_t pmm_get_free_pages(void);

/** @brief Get the number of used pages.
 *  @returns The number of used pages.
 */
size_t pmm_get_allocated_pages(void);

#endif /* SRC_GAIA_PMM_H */
