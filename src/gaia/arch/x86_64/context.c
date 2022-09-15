/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/host.h"
#include "paging.h"
#include <context.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>

void context_init(Context **context, bool user)
{
    *context = slab_alloc(sizeof(Context));

    Context *ctx = *context;

    ctx->pagemap.pml4 = pmm_alloc_zero();

    assert(ctx->pagemap.pml4 != NULL);

    ctx->frame.cs = user ? 0x43 : 0x28;
    ctx->frame.ss = user ? 0x3b : 0x30;
    ctx->frame.rflags = 0x202;

    uint64_t *new_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)ctx->pagemap.pml4));
    uint64_t *old_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)paging_get_kernel_pagemap()->pml4));

    for (size_t i = 256; i < 512; i++)
    {
        new_pml4[i] = old_pml4[i];
    }
}

void context_start(Context *context, uintptr_t entry_point, uintptr_t stack_pointer, bool alloc_stack)
{
    context->frame.rip = entry_point;
    context->frame.rsp = stack_pointer;

    if (alloc_stack)
    {
        uint64_t *stack = pmm_alloc_zero();

        context->stack_low_half = host_phys_to_virt((uintptr_t)stack);

        host_map_page(&context->pagemap, stack_pointer - 0x1000, (uintptr_t)stack, PAGE_USER | PAGE_WRITABLE);

        stack = pmm_alloc_zero();

        host_map_page(&context->pagemap, stack_pointer, (uintptr_t)stack + 0x1000, PAGE_USER | PAGE_WRITABLE);
        context->stack_high_half = host_phys_to_virt((uintptr_t)stack + 0x1000);
    }
}

void context_save(Context *context, InterruptStackframe *frame)
{
    context->frame = *frame;
}

void context_switch(Context *context, InterruptStackframe *frame)
{

    paging_load_pagemap(&context->pagemap);
    *frame = context->frame;
}

void context_copy(Context *dest, Context *src)
{
    assert(dest != src);

    memcpy((void *)(dest->stack_low_half), (void *)(src->stack_low_half), 0x1000);
    memcpy((void *)(dest->stack_high_half - 0x1000), (void *)(src->stack_high_half - 0x1000), 0x1000);

    memcpy(dest, src, sizeof(Context));

    dest->pagemap = src->pagemap;

    paging_copy_pagemap((uint64_t *)(host_phys_to_virt((uintptr_t)dest->pagemap.pml4)), (uint64_t *)(host_phys_to_virt((uintptr_t)src->pagemap.pml4)), 256, 3);

    dest->frame.rax = 0;
}
