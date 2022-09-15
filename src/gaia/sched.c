/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/ports.h"
#include <context.h>
#include <gaia/elf.h>
#include <gaia/host.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/spinlock.h>
#include <gaia/vec.h>
#include <paging.h>

typedef Vec(Task *) TaskVec;

static Spinlock lock = {0};
static size_t current_pid = -1;
static TaskVec running_tasks = {0};
static bool restore_frame = false;
static Task *current_task = NULL;
static Task *idle_task = NULL;

static void idle(void)
{
    while (true)
    {
    }
}

void sched_init(void)
{
    lock = (Spinlock){0};
    vec_init(&running_tasks);

    idle_task = sched_create_new_task(false);
    void *stack = pmm_alloc_zero();

    context_start(idle_task->context, (uintptr_t)idle, (uintptr_t)stack + MMAP_IO_BASE, false);

    current_task = idle_task;
    restore_frame = false;
}

Task *sched_create_new_task(bool user)
{
    lock_acquire(&lock);

    Task *ret = slab_alloc(sizeof(Task));

    ret->state = RUNNING;
    ret->pid = current_pid++;
    ret->namespace = slab_alloc(sizeof(PortNamespace));

    vec_init(&ret->namespace->bindings);

    PortBinding null_binding = {PORT_NULL, 0, 0};
    vec_push(&ret->namespace->bindings, null_binding);
    ret->namespace->current_name = 1;

    context_init(&ret->context, user);

    if (user)
    {
        vec_push(&running_tasks, ret);
    }

    lock_release(&lock);

    return ret;
}

Task *sched_create_new_task_from_elf(uint8_t *data)
{
    Task *ret = sched_create_new_task(true);

    uintptr_t pc;

    elf_load(data, &pc, ret->context);

    context_start(ret->context, pc, USER_STACK_TOP, true);

    log("Created new elf task with pid %d and rip %p", ret->pid, ret->context->frame.rip);

    return ret;
}

void sched_switch_to_task(size_t pid)
{
    // lock_acquire(&lock);

    current_task = running_tasks.data[pid];
    // prev_task = pid;

    // lock_release(&lock);
}

static Task *get_and_remove(size_t index)
{
    Task *ret = running_tasks.data[index];
    running_tasks.length--;

    for (size_t i = index; i < running_tasks.length; i++)
    {
        running_tasks.data[i] = running_tasks.data[i + 1];
    }

    return ret;
}

void sched_tick(InterruptStackframe *frame)
{
    lock_acquire(&lock);

    if (restore_frame)
    {
        context_save(current_task->context, frame);
        // warn("New rip of %d is %p", current_task->pid, frame->rip);
        vec_push(&running_tasks, current_task);
    }
    else
    {
        restore_frame = true;
    }

    bool found_task = false;

    for (size_t i = 0; i < running_tasks.length; i++)
    {
        if (running_tasks.data[i]->state == RUNNING)
        {
            current_task = get_and_remove(i);
            found_task = true;
            break;
        }
    }

    lock_release(&lock);

    if (!found_task)
    {
        restore_frame = false;
        current_task = idle_task;
    }

    //  log("Running task %d with rip %p", current_task->pid, current_task->context->frame.rip);
    context_switch(current_task->context, frame);
}

Task *sched_get_current_task(void)
{
    return current_task;
}
