/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "gaia/host.h"
#include <asm.h>
#include <gaia/firmware/lapic.h>
#include <gaia/sched.h>
#include <gaia/syscall.h>
#include <gaia/term.h>
#include <gaia/vmm.h>
#include <idt.h>
#include <paging.h>

static const char *exception_strings[32] = {
    "Division by zero (learn maths nerd)",
    "Debug (thanks for debugging)",
    "Non-maskable Interrupt (bro what)",
    "Breakpoint (yes)",
    "Overflow (your mom too big)",
    "Bound Range Exceeded (Use rust)",
    "Invalid Opcode (what did you do)",
    "Device Not Available (I don't know what this means)",
    "Double Fault (You f*cked up)",
    "Coprocessor Segment Overrun (I don't know what this means)",
    "Invalid TSS (I f*cked up)",
    "Segment Not Present (What did you do)",
    "Stack-Segment Fault (What did you do)",
    "General Protection Fault (Trying to be smart, huh?)",
    "Page Fault (Rewrite in rust)",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"};

static void do_backtrace(InterruptStackframe *frame)
{
    uintptr_t faulting_address = asm_read_cr2();

    term_blue_screen();

    bool in_task = sched_get_current_task() != NULL;

    if (in_task)
    {
        in_task = sched_get_current_task()->pid != (size_t)-1;
    }

    debug_print("*** Fatal crash ***");

    if (in_task)
    {
        debug_print("In process with PID %d\n", sched_get_current_task()->pid);
    }
    else
    {
        debug_print("In kernel\n");
    }

    debug_print("%s at PC=%p (error code = 0x%p)", exception_strings[frame->intno], frame->rip, frame->err);
    debug_print("   RAX=%p RBX=%p RCX=%p RDX=%p", frame->rax, frame->rbx, frame->rcx, frame->rdx);
    debug_print("   RSI=%p RDI=%p RBP=%p RSP=%p", frame->rsi, frame->rdi, frame->rbp, frame->rsp);
    debug_print("   R8=%p  R9=%p  R10=%p R11=%p", frame->r8, frame->r9, frame->r10, frame->r11);
    debug_print("   R12=%p R13=%p R14=%p R15=%p", frame->r12, frame->r13, frame->r14, frame->r15);
    debug_print("   CR0=%p CR2=%p CR3=%p RIP=%p\n", asm_read_cr0(), faulting_address, asm_read_cr3(), frame->rip);

    if (frame->intno == 0xE)
    {
        debug_print("Page fault at address %p\n", faulting_address);
    }

    if (faulting_address != frame->rip && frame->intno != 0xE)
    {
        debug_print("Faulting opcode chain: %p (big endian)\n", *(uint64_t *)frame->rip);
    }

    if (in_task && frame->intno != 0xD)
    {
        debug_print("User mappings:");

        VmMapping *mapping = context_get_space(sched_get_current_task()->context)->mappings;

        while (mapping)
        {
            debug_print("    %p (size=%p)", mapping->address, mapping->object.size);
            mapping = mapping->next;
        }

        debug_print("\n");
    }

    debug_print("If this wasn't intentional, please report an issue on https://github.com/nyx-org/nyx");
    host_disable_interrupts();
    host_hang();
}

uintptr_t interrupts_handler(uint64_t rsp)
{
    InterruptStackframe *stack = (InterruptStackframe *)rsp;

    bool pf_ok = false;

    if (stack->intno == 0xe)
    {
        if (sched_get_current_task())
        {
            pf_ok = vmm_page_fault_handler(context_get_space(sched_get_current_task()->context), asm_read_cr2());
        }
    }

    if (!pf_ok && stack->intno < 32)
    {

        do_backtrace(stack);
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
