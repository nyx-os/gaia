/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file charon.h
 * @brief Generic boot protocol
 */

#ifndef SRC_KERN_CHARON_H_
#define SRC_KERN_CHARON_H_
#include <libkern/base.h>

#define CHARON_MODULE_MAX 16
#define CHARON_MMAP_SIZE_MAX 128

/**
 * Memory map entry type
 */
enum charon_mmap_entry_type {
    MMAP_FREE,
    MMAP_RESERVED,
    MMAP_MODULE,
    MMAP_RECLAIMABLE,
    MMAP_FRAMEBUFFER
};

typedef struct {
    uintptr_t base;
    size_t size;
    enum charon_mmap_entry_type type;
} charon_mmap_entry_t;

typedef struct {
    uint8_t count;
    charon_mmap_entry_t entries[CHARON_MMAP_SIZE_MAX];
} charon_mmap_t;

typedef struct {
    bool present : 1;
    uintptr_t address;
    uint32_t width, height, pitch, bpp;
} charon_framebuffer_t;

typedef struct {
    const char *name;
    size_t size;
    uintptr_t address;
} charon_module_t;

typedef struct {
    uint16_t count;
    charon_module_t modules[CHARON_MODULE_MAX];
} charon_modules_t;

typedef struct {
    uintptr_t rsdp;
    charon_framebuffer_t framebuffer;
    charon_mmap_t memory_map;
    charon_modules_t modules;
} charon_t;

#endif
