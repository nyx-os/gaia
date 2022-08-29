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

typedef struct
{
    InterruptStackframe frame;
    Pagemap pagemap;
    size_t pid;
    uintptr_t stack_top;
} Task;

#define USER_STACK_TOP 0x7fffffffe000

void sched_switch_to_next_task(InterruptStackframe *frame);

Task *sched_create_new_task_from_elf(uint8_t *data);

Task *sched_get_current_task(void);

void sched_init(void);

#endif /* ARCH_X86_64_SCHED_H */
