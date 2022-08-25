/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef ARCH_X86_64_IDT_H
#define ARCH_X86_64_IDT_H
#include <gaia/base.h>

#define INTGATE 0x8e
#define TRAPGATE 0xef

typedef struct PACKED
{
    uint16_t size;
    uint64_t addr;
} IdtPointer;

typedef struct PACKED
{
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t zero;
} IdtDescriptor;

void idt_initialize(void);

#endif /* ARCH_X86_64_IDT_H */
