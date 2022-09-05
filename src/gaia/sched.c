/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/elf.h>
#include <gaia/host.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/spinlock.h>
#include <gaia/vec.h>
#include <paging.h>

typedef Vec(Task) TaskVec;

static Spinlock lock = {0};
static size_t current_pid = 0;
static TaskVec tasks = {0};
static long long current_task = -1;

void sched_init(void)
{
    lock = (Spinlock){0};
    vec_init(&tasks);
}

Task sched_create_new_task_from_elf(uint8_t *data)
{
    lock_acquire(&lock);

    Task ret = {0};

    ret.pid = current_pid++;

    context_init(&ret.context);

    uintptr_t pc;

    elf_load(data, &pc, ret.context);

    context_start(ret.context, pc, USER_STACK_TOP);

    vec_push(&tasks, ret);

    lock_release(&lock);

    return ret;
}

static long long prev_task = -1;
static size_t tick = 0;

void sched_tick(InterruptStackframe *frame)
{
    lock_acquire(&lock);

    long long task_to_run = current_task;

    if (prev_task != -1)
    {
        context_save(tasks.data[prev_task].context, frame);
    }

    if (++tick >= 5)
    {
        tick = 0;

        if (++current_task >= (long long)tasks.length)
        {
            current_task = 0;
        }

        task_to_run = current_task;

        prev_task = current_task;
    }

    if (task_to_run != -1)
    {
        context_switch(tasks.data[task_to_run].context, frame);
    }

    lock_release(&lock);
}

Task sched_get_current_task(void)
{
    return tasks.data[current_task];
}
