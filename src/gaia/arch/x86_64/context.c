/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/host.h"
#include "gaia/vmm.h"
#include "paging.h"
#include <context.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <simd.h>

void context_init(Context *ctx, bool user)
{
    ctx->space = slab_alloc(sizeof(VmmMapSpace));
    ctx->pagemap.pml4 = pmm_alloc_zero();
    ctx->fpu_state = (void *)(host_phys_to_virt((uintptr_t)pmm_alloc_zero()));

    simd_initialize_context(ctx->fpu_state);

    assert(ctx->pagemap.pml4 != NULL);

    vmm_space_init(ctx->space);

    ctx->space->pagemap = &ctx->pagemap;
    ctx->space->bump = MMAP_BUMP_BASE;

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

    if (!stack_pointer && alloc_stack)
    {
        context->frame.rsp = USER_STACK_TOP;
    }

    if (alloc_stack)
    {
        VmCreateArgs args = {.addr = 0, .size = MIB(8)};
        VmObject stack = vm_create(args);
        VmMapArgs map_args = {.object = &stack, .vaddr = context->frame.rsp - MIB(8), .flags = VM_MAP_FIXED, .protection = VM_PROT_READ | VM_PROT_WRITE};
        vm_map(context->space, map_args);
    }
}

void context_save(Context *context, InterruptStackframe *frame)
{
    simd_save_state(context->fpu_state);
    context->frame = *frame;
}

void context_switch(Context *context, InterruptStackframe *frame)
{
    simd_restore_state(context->fpu_state);
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

    //    paging_copy_pagemap((uint64_t *)(host_phys_to_virt((uintptr_t)dest->pagemap.pml4)), (uint64_t *)(host_phys_to_virt((uintptr_t)src->pagemap.pml4)), 256, 3);

    dest->frame.rax = 0;
}

VmmMapSpace *context_get_space(Context *ctx)
{
    return ctx->space;
}
