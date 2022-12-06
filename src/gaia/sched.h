/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_SCHED_H
#define SRC_GAIA_SCHED_H

#include <gaia/host.h>
#include <gaia/ports.h>
#include <gaia/rights.h>
#include <gaia/vmm.h>
#include <idt.h>
#include <paging.h>

typedef struct Context Context;

typedef enum
{
    STOPPED,
    BLOCKED,
    RUNNING,
} TaskState;

typedef struct
{
    Context *context;
    size_t pid;
    uintptr_t stack_bottom;
    PortNamespace *namespace;
    Rights rights;
    TaskState state;
} Task;

#define USER_STACK_TOP 0x7fffffffe000

void context_init(Context *context, bool user);
void context_start(Context *context, uintptr_t entry_point, uintptr_t stack_pointer, bool alloc_stack);
void context_save(Context *context, InterruptStackframe *frame);
void context_switch(Context *context, InterruptStackframe *frame);
void context_copy(Context *dest, Context *src);

VmmMapSpace *context_get_space(Context *context);

void sched_tick(InterruptStackframe *frame);

Task *sched_create_new_task(bool user, Rights rights);

Task *sched_create_new_task_from_elf(uint8_t *data, Rights rights);
void sched_save_state(InterruptStackframe *frame);

void sched_switch_to_task(size_t pid);

Task *sched_get_current_task(void);

Task *sched_lookup_task(size_t pid);

void sched_init(void);

#endif /* ARCH_X86_64_SCHED_H */
