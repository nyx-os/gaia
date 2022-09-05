/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/host.h"
#include <context.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>

void context_init(Context **context)
{
    *context = slab_alloc(sizeof(Context));

    Context *ctx = *context;

    ctx->pagemap.pml4 = pmm_alloc_zero();

    assert(ctx->pagemap.pml4 != NULL);

    ctx->frame.cs = 0x43;
    ctx->frame.ss = 0x3b;
    ctx->frame.rflags = 0x202;

    uint64_t *new_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)ctx->pagemap.pml4));
    uint64_t *old_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)paging_get_kernel_pagemap()->pml4));

    for (size_t i = 256; i < 512; i++)
    {
        new_pml4[i] = old_pml4[i];
    }
}

void context_start(Context *context, uintptr_t entry_point, uintptr_t stack_pointer)
{
    context->frame.rip = entry_point;
    context->frame.rsp = stack_pointer;

    uint64_t *stack = pmm_alloc_zero();

    host_map_page(&context->pagemap, stack_pointer - 0x1000, (uintptr_t)stack, PAGE_USER | PAGE_WRITABLE);

    stack = pmm_alloc_zero();

    host_map_page(&context->pagemap, stack_pointer, (uintptr_t)stack + 0x1000, PAGE_USER | PAGE_WRITABLE);
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
