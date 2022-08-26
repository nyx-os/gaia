/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include <gaia/firmware/lapic.h>
#include <idt.h>

extern uintptr_t __interrupt_vector[];
static IdtDescriptor idt[256] = {0};

static IdtDescriptor idt_make_entry(uint64_t offset, uint8_t type)
{
    return (IdtDescriptor){
        .offset_lo = offset & 0xFFFF,
        .selector = 0x28,
        .ist = 0,
        .type_attr = type,
        .offset_mid = (offset >> 16) & 0xFFFF,
        .offset_hi = (offset >> 32) & 0xFFFFFFFF,
        .zero = 0};
}

static void install_isrs(void)
{
    for (int i = 0; i < 256; i++)
    {
        idt[i] = idt_make_entry(__interrupt_vector[i], INTGATE);
    }
}

static void idt_reload(void)
{

    IdtPointer idt_pointer = {0};
    idt_pointer.size = sizeof(idt) - 1;
    idt_pointer.addr = (uintptr_t)idt;

    __asm__ volatile("lidt %0"
                     :
                     : "m"(idt_pointer));
}

struct PACKED InterruptStackframe
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t intno;
    uint64_t err;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

uintptr_t interrupts_handler(InterruptStackframe *stack)
{
    lapic_eoi();

    return (uintptr_t)stack;
}

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

void idt_initialize()
{
    install_isrs();
    idt_reload();

    // disable PIC
    host_out8(PIC1_DATA, 0xff);
    host_out8(PIC2_DATA, 0xff);
}
