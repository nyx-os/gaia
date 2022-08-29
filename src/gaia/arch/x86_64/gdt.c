/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/pmm.h"
#include <gaia/base.h>
#include <gdt.h>

typedef struct PACKED
{
    uint16_t limit;
    uintptr_t base;
} GdtPointer;

typedef struct PACKED
{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t granularity;
    uint8_t base2;
} GdtDescriptor;

typedef struct PACKED
{
    uint16_t length;
    uint16_t base0;
    uint8_t base1;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base2;
    uint32_t base3;
    uint32_t reserved;
} TssDescriptor;

typedef struct
{
    GdtDescriptor entries[9];
    TssDescriptor tss;
} Gdt;

typedef struct PACKED
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} Tss;

static Gdt gdt = {
    {{0x0000, 0, 0, 0x00, 0x00, 0},       // NULL
     {0xFFFF, 0, 0, 0x9A, 0x00, 0},       // 16 Bit code
     {0xFFFF, 0, 0, 0x92, 0x00, 0},       // 16 Bit data
     {0xFFFF, 0, 0, 0x9A, 0xCF, 0},       // 32 Bit code
     {0xFFFF, 0, 0, 0x92, 0xCF, 0},       // 32 Bit data
     {0x0000, 0, 0, 0x9A, 0x20, 0},       // 64 Bit code
     {0x0000, 0, 0, 0x92, 0x00, 0},       // 64 Bit data
     {0x0000, 0, 0, 0xF2, 0x00, 0},       // User data
     {0x0000, 0, 0, 0xFA, 0x20, 0}},      // User code
    {0x0000, 0, 0, 0x89, 0x00, 0, 0, 0}}; // TSS

static GdtPointer gdt_pointer = {
    .limit = sizeof(gdt) - 1,
    .base = (uint64_t)&gdt};

static Tss tss = {0};

extern void gdt_load(GdtPointer *pointer);
extern void tss_reload(void);

static TssDescriptor make_tss_entry(uintptr_t tss)
{
    return (TssDescriptor){
        .length = sizeof(TssDescriptor),
        .base0 = (uint16_t)(tss & 0xffff),
        .base1 = (uint8_t)((tss >> 16) & 0xff),
        .flags1 = 0x89,
        .flags2 = 0,
        .base2 = (uint8_t)((tss >> 24) & 0xff),
        .base3 = (uint32_t)(tss >> 32),
        .reserved = 0,
    };
}

void gdt_initialize()
{

    tss.rsp0 = (uint64_t)host_phys_to_virt((uintptr_t)pmm_alloc_zero()) + PAGE_SIZE;
    tss.iopb_offset = sizeof(Tss);
    gdt.tss = make_tss_entry((uintptr_t)&tss);

    gdt_load(&gdt_pointer);
    tss_reload();
}
