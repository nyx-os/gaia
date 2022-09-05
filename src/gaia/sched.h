/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef ARCH_X86_64_SCHED_H
#define ARCH_X86_64_SCHED_H

#include <gaia/host.h>
#include <idt.h>
#include <paging.h>

typedef struct Context Context;

typedef struct
{
    Context *context;
    size_t pid;
    uintptr_t stack_top;
} Task;

#define USER_STACK_TOP 0x7fffffffe000

void context_init(Context **context);
void context_start(Context *context, uintptr_t entry_point, uintptr_t stack_pointer);
void context_save(Context *context, InterruptStackframe *frame);
void context_switch(Context *context, InterruptStackframe *frame);

void sched_tick(InterruptStackframe *frame);

Task sched_create_new_task_from_elf(uint8_t *data);

Task sched_get_current_task(void);

void sched_init(void);

#endif /* ARCH_X86_64_SCHED_H */
