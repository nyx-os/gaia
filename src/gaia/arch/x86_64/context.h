/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef ARCH_X86_64_CONTEXT_H
#define ARCH_X86_64_CONTEXT_H
#include <gaia/vmm.h>
#include <idt.h>
#include <paging.h>

struct Context
{
    InterruptStackframe frame;
    Pagemap pagemap;
    uintptr_t stack_low_half, stack_high_half;
    VmmMapSpace *space;
    void *fpu_state;
};

#endif /* ARCH_X86_64_CONTEXT_H */
