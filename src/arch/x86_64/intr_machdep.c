/* SPDX-License-Identifier: BSD-2-Clause */
#include "kern/syscall.h"
#include <libkern/base.h>
#include <machdep/intr.h>
#include <x86_64/asm.h>
#include <posix/tty.h>

#include <kern/sched.h>

#define INTGATE 0x8e
#define TRAPGATE 0xef

typedef struct PACKED {
    uint16_t size;
    uint64_t addr;
} idt_pointer_t;

typedef struct PACKED {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t zero;
} idt_descriptor_t;

extern uintptr_t __interrupt_vector[];
static idt_descriptor_t idt[256] = { 0 };
static intr_handler_t handlers[256] = { NULL };

static idt_descriptor_t idt_make_entry(uint64_t offset, uint8_t type,
                                       uint8_t ring)
{
    return (idt_descriptor_t){ .offset_lo = offset & 0xFFFF,
                               .selector = 0x28,
                               .ist = 0,
                               .type_attr = (uint8_t)(type | ring << 5),
                               .offset_mid = (offset >> 16) & 0xFFFF,
                               .offset_hi = (offset >> 32) & 0xFFFFFFFF,
                               .zero = 0 };
}

static void install_isrs(void)
{
    for (int i = 0; i < 256; i++) {
        idt[i] = idt_make_entry(__interrupt_vector[i], INTGATE, 0);
    }

    idt[0x80] = idt_make_entry(__interrupt_vector[0x80], INTGATE, 3);
}

static idt_pointer_t idtr = { 0 };

static void idt_reload(void)
{
    idtr.size = sizeof(idt) - 1;
    idtr.addr = (uintptr_t)idt;

    asm volatile("lidt %0" : : "m"(idtr));
}

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

extern void timer_interrupt(void);

void idt_init(void)
{
    install_isrs();
    idt_reload();

    idt[0x20] = idt_make_entry((uintptr_t)timer_interrupt, INTGATE, 0);

    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void intr_register(int vec, intr_handler_t handler)
{
    handlers[vec] = handler;
}

void lapic_eoi(void);

/* Faster dispatching this way */
uint64_t intr_timer_handler(uint64_t rsp)
{
    sched_tick((intr_frame_t *)rsp);
    lapic_eoi();
    return rsp;
}

static void do_exception(intr_frame_t *frame)
{
    error("exception: 0x%zx, err=0x%zx", frame->intno, frame->err);
    error("RAX=0x%zx RBX=0x%zx RCX=0x%zx RDX=0x%zx", frame->rax, frame->rbx,
          frame->rcx, frame->rdx);
    error("RSI=0x%zx RDI=0x%zx RBP=0x%zx RSP=0x%zx", frame->rsi, frame->rdi,
          frame->rbp, frame->rsp);
    error("R8=0x%zx  R9=0x%zx  R10=0x%zx R11=0x%zx", frame->r8, frame->r9,
          frame->r10, frame->r11);
    error("R12=0x%zx R13=0x%zx R14=0x%zx R15=0x%zx", frame->r12, frame->r13,
          frame->r14, frame->r15);
    error("CR0=0x%zx CR2=0x%zx CR3=0x%zx RIP=0x%zx", read_cr0(), read_cr2(),
          read_cr3(), frame->rip);
    panic("If this wasn't intentional, please report an issue on https://github.com/nyx-org/nyx");
}

uint64_t interrupts_handler(uint64_t rsp)
{
    intr_frame_t *frame = (intr_frame_t *)rsp;

    if (frame->intno < 0x20 && frame->intno != 0xe) {
        do_exception(frame);
    }

    if (frame->intno == 0xe) {
        if (vm_fault(&sched_curr()->parent->map, read_cr2()) < 0) {
            do_exception(frame);
        }
    }

    if (handlers[frame->intno]) {
        handlers[frame->intno](frame);
    }

    if (frame->intno == 0x80) {
        syscall_frame_t sys_frame = { 0 };

        sys_frame.num = frame->rax;
        sys_frame.param1 = frame->rdi;
        sys_frame.param2 = frame->rsi;
        sys_frame.param3 = frame->rdx;
        sys_frame.param4 = frame->r10;
        sys_frame.param5 = frame->r8;
        sys_frame.frame = frame;

        frame->rax = syscall_handler(sys_frame);
    } else {
        lapic_eoi();
    }

    return rsp;
}
