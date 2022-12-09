/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/lapic.h>
#include <gaia/sched.h>
#include <gaia/syscall.h>
#include <gaia/vmm.h>
#include <idt.h>
#include <paging.h>

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

    bool pf_ok = false;

    if (stack->intno == 0xe)
    {
        if (sched_get_current_task())
        {
            pf_ok = vmm_page_fault_handler(context_get_space(sched_get_current_task()->context), read_cr2());
        }
    }

    if (!pf_ok && stack->intno < 32)
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
        uintptr_t *ret = (uintptr_t *)((uint8_t *)stack + offsetof(InterruptStackframe, rax));
        uintptr_t *errno = (uintptr_t *)((uint8_t *)stack + offsetof(InterruptStackframe, rbx));

        SyscallFrame frame = {stack->rdi, stack->rsi, stack->rdx, stack->rcx, stack->r8, ret, errno, stack->rip, stack};

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
