/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

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

uintptr_t interrupts_handler(uintptr_t stack_pointer)
{
    //   log("e");
    lapic_eoi();
    return stack_pointer;
}
