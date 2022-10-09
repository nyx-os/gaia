/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @file
 * @brief Generic boot protocol.
 */

#ifndef SRC_GAIA_CHARON_H
#define SRC_GAIA_CHARON_H
#include <gaia/base.h>

#define CHARON_MODULE_MAX 16
#define CHARON_MMAP_SIZE_MAX 128

typedef enum
{
    MMAP_FREE,
    MMAP_RESERVED,
    MMAP_MODULE,
    MMAP_RECLAIMABLE,
    MMAP_FRAMEBUFFER
} CharonMemoryMapEntryType;

typedef struct PACKED
{

    uintptr_t base;
    size_t size;
    CharonMemoryMapEntryType type;
} CharonMemoryMapEntry;

typedef struct PACKED
{
    uint8_t count;
    CharonMemoryMapEntry entries[CHARON_MMAP_SIZE_MAX];
} CharonMemoryMap;

typedef struct PACKED
{
    bool present;
    uintptr_t address;
    uint32_t width, height, pitch, bpp;
} CharonFramebuffer;

typedef struct PACKED
{
    uint32_t size;
    uintptr_t address;
    const char name[16];
} CharonModule;

typedef struct PACKED
{
    uint8_t count;
    CharonModule modules[CHARON_MODULE_MAX];
} CharonModules;

typedef struct PACKED
{
    uintptr_t rsdp;
    CharonFramebuffer framebuffer;
    CharonModules modules;
    CharonMemoryMap memory_map;
} Charon;

Charon *gaia_get_charon(void);

#endif /* SRC_GAIA_CHARON_H */
