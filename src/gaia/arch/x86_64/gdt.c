/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/base.h>
#include <gdt.h>

// TODO: Maybe rewrite this file to make it more readable and less hardcoded.
// Right now we're just hardcoding everything because I was lazy when I first wrote the file, as it's mostly boilerplate and legacy cruft.

// This function is implemented in assembly, and takes the address of the gdt pointer (Gdt pointer)
extern void gdt_load(void *);

// These are magic values for the GDT (thanks pitust).
// They follow the Limine specification so that we can still use the Limine terminal during early boot.
static uint64_t gdt[] = {
    0x0000000000000000, // Null segment
    0x00009a000000ffff, // 16-bit code segment
    0x000093000000ffff, // 16-bit data segment
    0x00cf9a000000ffff, // 32-bit code segment
    0x00cf93000000ffff, // 32-bit data segment
    0x00af9b000000ffff, // 64-bit code segment
    0x00af93000000ffff, // 64-bit data segment
    0x00affb000000ffff, // 64-bit user code segment
    0x00aff3000000ffff  // 64-bit user data segment
};

// This is awful.
static struct PACKED
{
    uint16_t limit;
    uint64_t base;
} gdt_pointer = {
    .limit = sizeof(gdt) - 1,
    .base = (uint64_t)&gdt};

void gdt_initialize()
{
    gdt_load(&gdt_pointer);
}
