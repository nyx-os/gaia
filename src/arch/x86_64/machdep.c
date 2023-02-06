/* SPDX-License-Identifier: BSD-2-Clause */
#include <machdep/machdep.h>
#include <x86_64/asm.h>

void machine_dbg_putc(int c, void *ctx)
{
    DISCARD(ctx);
    outb(0xe9, c);
}

void machine_init(void)
{
}
