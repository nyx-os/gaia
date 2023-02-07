/* SPDX-License-Identifier: BSD-2-Clause */
#include <libkern/base.h>
#include <kern/vm/phys.h>

typedef struct PACKED {
    uint16_t limit;
    uintptr_t base;
} gdt_pointer_t;

typedef struct PACKED {
    uint16_t limit0;
    uint16_t base0;
    uint8_t base1;
    uint8_t access;
    uint8_t granularity;
    uint8_t base2;
} gdt_descriptor_t;

typedef struct PACKED {
    uint16_t length;
    uint16_t base0;
    uint8_t base1;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base2;
    uint32_t base3;
    uint32_t reserved;
} tss_descriptor_t;

typedef struct {
    gdt_descriptor_t entries[9];
    tss_descriptor_t tss;
} gdt_t;

typedef struct PACKED {
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
} tss_t;

static gdt_t gdt = { { { 0x0000, 0, 0, 0x00, 0x00, 0 }, // NULL
                       { 0xFFFF, 0, 0, 0x9A, 0x00, 0 }, // 16 Bit code
                       { 0xFFFF, 0, 0, 0x92, 0x00, 0 }, // 16 Bit data
                       { 0xFFFF, 0, 0, 0x9A, 0xCF, 0 }, // 32 Bit code
                       { 0xFFFF, 0, 0, 0x92, 0xCF, 0 }, // 32 Bit data
                       { 0x0000, 0, 0, 0x9A, 0x20, 0 }, // 64 Bit code
                       { 0x0000, 0, 0, 0x92, 0x00, 0 }, // 64 Bit data
                       { 0x0000, 0, 0, 0xF2, 0x00, 0 }, // User data
                       { 0x0000, 0, 0, 0xFA, 0x20, 0 } }, // User code
                     { 0x0000, 0, 0, 0x89, 0x00, 0, 0, 0 } }; // TSS

static gdt_pointer_t gdt_pointer = { .limit = sizeof(gdt) - 1,
                                     .base = (uint64_t)&gdt };

static tss_t tss = { 0 };

extern void gdt_load(gdt_pointer_t *pointer);
extern void tss_reload(void);

static tss_descriptor_t make_tss_entry(uintptr_t tss)
{
    return (tss_descriptor_t){
        .length = sizeof(tss_descriptor_t),
        .base0 = (uint16_t)(tss & 0xffff),
        .base1 = (uint8_t)((tss >> 16) & 0xff),
        .flags1 = 0x89,
        .flags2 = 0,
        .base2 = (uint8_t)((tss >> 24) & 0xff),
        .base3 = (uint32_t)(tss >> 32),
        .reserved = 0,
    };
}

void gdt_init(void)
{
    tss.rsp0 = (uint64_t)P2V((uintptr_t)phys_allocz()) + PAGE_SIZE;
    tss.iopb_offset = sizeof(tss_t);
    gdt.tss = make_tss_entry((uintptr_t)&tss);

    gdt_load(&gdt_pointer);
    tss_reload();
}
