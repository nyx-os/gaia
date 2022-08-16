/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/** @file
 *  @brief x86_64 GDT (Global Descriptor Table) definitions.
 */

#ifndef ARCH_X86_64_GDT_H
#define ARCH_X86_64_GDT_H

/**
 * @brief initialize and load the GDT.
 *
 */
void gdt_initialize(void);

#endif /* ARCH_X86_64_GDT_H */
