/* SPDX-License-Identifier: BSD-2-Clause */
#include <machdep/machdep.h>
#include <x86_64/asm.h>
#include <x86_64/apic.h>
#include <kern/term.h>

void machine_dbg_putc(int c, void *ctx)
{
    DISCARD(ctx);
    outb(0xe9, c);
}

void gdt_init(void);
void idt_init(void);

void machine_init(void)
{
    gdt_init();
    idt_init();
    apic_init();
}
