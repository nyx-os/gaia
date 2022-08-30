/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/elf.h>
#include <gaia/host.h>
#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/spinlock.h>
#include <gaia/vec.h>
#include <paging.h>
#include <sched.h>

/* TODO: make this arch-independent */

typedef Vec(Task *) TaskVec;

static Spinlock lock = {0};
static size_t current_pid = 0;
static TaskVec tasks = {0};
static size_t current_task = 0;

void sched_init(void)
{
    lock = (Spinlock){0};
    vec_init(&tasks);
}

Task *sched_create_new_task_from_elf(uint8_t *data)
{
    lock_acquire(&lock);

    Task *ret = slab_alloc(sizeof(Task));
    ret->pagemap.lock = 0;
    ret->pagemap.pml4 = pmm_alloc_zero();

    assert(ret->pagemap.pml4 != NULL);
    ret->pid = current_pid++;
    ret->frame = (InterruptStackframe){0};

    uint64_t *new_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)ret->pagemap.pml4));
    uint64_t *old_pml4 = (uint64_t *)(host_phys_to_virt((uintptr_t)paging_get_kernel_pagemap()->pml4));

    for (size_t i = 256; i < 512; i++)
    {
        new_pml4[i] = old_pml4[i];
    }

    uint64_t *stack = pmm_alloc_zero();
    host_map_page(&ret->pagemap, USER_STACK_TOP, (uintptr_t)stack + 0x1000, PAGE_USER | PAGE_WRITABLE);

    elf_load(data, &ret->frame.rip, &ret->pagemap);

    ret->frame.rsp = USER_STACK_TOP;
    ret->frame.cs = 0x43;
    ret->frame.ss = 0x3b;
    ret->frame.rflags = 0x202;

    vec_push(&tasks, ret);

    lock_release(&lock);

    return ret;
}

static Task *prev_task = NULL;

void sched_switch_to_next_task(InterruptStackframe *frame)
{
    lock_acquire(&lock);

    if (prev_task)
    {
        prev_task->frame = *frame;
    }

    Task *task = tasks.data[current_task];

    prev_task = task;

    if (++current_task >= tasks.length)
    {
        current_task = 0;
    }

    *frame = task->frame;
    paging_load_pagemap(&task->pagemap);

    lock_release(&lock);
}

Task *sched_get_current_task(void)
{
    return tasks.data[current_task];
}
