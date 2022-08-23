/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file
 * @brief Host architecture specific definitions.
 */

#ifndef LIB_GAIA_HOST_H
#define LIB_GAIA_HOST_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Write a string to the debug output.
 *
 * This function is used for writing strings in the debugging functions such as log, panic and assert.
 *
 * @param str String to write.
 */
void host_debug_write_string(const char *str);

/**
 * @brief Freeze the system.
 *
 * This function hangs the host system, making further code execution impossible.
 *
 * @return This function should NOT return.
 */
void host_hang(void);

/**
 * @brief Read 8 bits (one byte) from io port.
 *
 * @param port The port to read from.
 * @return The data that was read
 */
uint8_t host_in8(uint16_t port);

/**
 * @brief Write 8 bits (one byte) to io port.
 *
 * @param port The port to write to.
 * @param data The data to write.
 */
void host_out8(uint16_t port, uint8_t data);

/**
 * @brief Read 16 bits (two bytes) from io port.
 *
 * @param port The port to read from.
 * @return The data that was read
 */
uint16_t host_in16(uint16_t port);

/**
 * @brief Write 16 bits (two bytes) to io port.
 *
 * @param port The port to write to.
 * @param data The data to write.
 */
void host_out16(uint16_t port, uint16_t data);

/**
 * @brief Read 32 bits (four bytes) from io port.
 *
 * @param port The port to read from.
 * @return The data that was read
 */
uint32_t host_in32(uint16_t port);

/**
 * @brief Write 32 bits (four bytes) to io port.
 *
 * @param port The port to write to.
 * @param data The data to write.
 */
void host_out32(uint16_t port, uint32_t data);

/**
 * @brief Enable interrupts.
 *
 */
void enable_interrupts(void);

/**
 * @brief Disable interrupts.
 *
 */
void disable_interrupts(void);

/**
 * @brief Convert a physical address to a virtual address.
 *
 * @param phys The address to convert.
 * @return uintptr_t The virtual address.
 */
uintptr_t host_phys_to_virt(uintptr_t phys);

/**
 * @brief Convert a physical address to a kernel virtual address.
 *
 * @param phys The address to convert.
 * @return uintptr_t The virtual address.
 */
uintptr_t host_phys_to_kern(uintptr_t phys);

/**
 * @brief Convert a virtual address to a physical address.
 *
 * @param virt The address to convert.
 * @return uintptr_t The physical address.
 */
uintptr_t host_virt_to_phys(uintptr_t virt);

/**
 * @brief Convert a kernel virtual address to a physical address.
 *
 * @param virt The address to convert.
 * @return uintptr_t The physical address.
 */
uintptr_t host_virt_to_kern(uintptr_t virt);

/**
 * @brief Initialize the host.
 *
 */
void host_initialize(void);

/**
 * @brief Get host name
 *
 */
const char *host_get_name(void);

/** Interrupt stack frame for the host architecture */
typedef struct InterruptStackframe InterruptStackframe;

/**
 * @brief Set interrupt handler for a specific interrupt.
 * @param interrupt The interrupt to set the handler for.
 * @param handler The handler.
 */
void host_set_interrupt_handler(uint8_t interrupt, void (*handler)(InterruptStackframe *));

typedef struct Pagemap Pagemap;

typedef enum
{
    PAGE_NONE = 0,
    PAGE_WRITABLE = (1 << 0),
    PAGE_NOT_EXECUTABLE = (1 << 1),
    PAGE_HUGE = (1 << 2),
} PageFlags;

/**
 * @brief Loads a pagemap.
 *
 * @param pagemap The pagemap to load.
 */
void host_load_pagemap(Pagemap *pagemap);

/**
 * @brief Maps a physical page to a virtual address.
 *
 * @param pagemap The pagemap in which the mapping should be made.
 * @param vaddr The virtual address to which the page should be mapped.
 * @param paddr The physical address of the page to map.
 * @param flags The flags to set for the mapping (See the PageFlags type).
 */
void host_map_page(Pagemap *pagemap, uintptr_t vaddr, uintptr_t paddr, PageFlags flags);

/**
 * @brief Unmaps a virtual address.
 *
 * @param pagemap The pagemap in which the mapping should be made.
 * @param vaddr The virtual address to which the page should be mapped.
 */
void host_unmap_page(Pagemap *pagemap, uintptr_t vaddr);

#endif /* GAIA_ARCH_HOST_H */
