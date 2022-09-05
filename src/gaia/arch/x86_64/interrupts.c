/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/host.h"
#include "paging.h"
#include <gaia/firmware/lapic.h>
#include <gaia/sched.h>
#include <gaia/syscall.h>
#include <idt.h>

static uint64_t read_cr2()
{
    uint64_t value;
    __asm__ volatile("mov %%cr2, %0"
                     : "=r"(value));
    return value;
}

uintptr_t interrupts_handler(uint64_t rsp)
{
    InterruptStackframe *stack = (InterruptStackframe *)rsp;

    if (stack->intno < 32)
    {
        panic("Exception 0x%x error code=0x%x RIP=%p RSP=%p CR2=%p", stack->intno, stack->err, stack->rip, stack->rsp, read_cr2());
        __builtin_unreachable();
    }

    switch (stack->intno)
    {
    case 32:
    {
        sched_tick(stack);
        break;
    }

    case 0x42:
    {
        SyscallFrame frame = {stack->rdi, stack->rsi, stack->rdx, stack->rcx, stack->r8};

        syscall(stack->rax, frame);
        break;
    }
    }

    if (stack->intno != 0x42)
    {
        lapic_eoi();
    }

    return rsp;
}
