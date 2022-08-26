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
#define CHARON_MMAP_SIZE_MAX 64

typedef enum
{
    MMAP_FREE,
    MMAP_RESERVED,
    MMAP_MODULE,
    MMAP_RECLAIMABLE,
    MMAP_FRAMEBUFFER
} CharonMemoryMapEntryType;

typedef struct
{

    uintptr_t base;
    size_t size;
    CharonMemoryMapEntryType type;
} CharonMemoryMapEntry;

typedef struct
{
    size_t count;
    CharonMemoryMapEntry entries[CHARON_MMAP_SIZE_MAX];
} CharonMemoryMap;

typedef struct
{
    bool present;
    uintptr_t address;
    uint32_t width, height, pitch, bpp;
} CharonFramebuffer;

typedef struct
{
    size_t size;
    uintptr_t address;
    const char *name;
} CharonModule;

typedef struct
{
    uint16_t count;
    CharonModule modules[CHARON_MODULE_MAX];
} CharonModules;

typedef struct
{
    uintptr_t rsdp;
    CharonFramebuffer framebuffer;
    CharonModules modules;
    CharonMemoryMap memory_map;
} Charon;

#endif /* SRC_GAIA_CHARON_H */
