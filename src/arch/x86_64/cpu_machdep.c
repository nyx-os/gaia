/* SPDX-License-Identifier: BSD-2-Clause */
#include <machdep/cpu.h>
#include <x86_64/asm.h>

void cpu_halt(void)
{
    while (true) {
        hlt();
    }
}

void cpu_enable_interrupts(void)
{
    sti();
}

void cpu_switch_context(intr_frame_t *frame, cpu_context_t ctx)
{
    *frame = ctx.regs;
}

void cpu_save_context(intr_frame_t *frame, cpu_context_t *ctx)
{
    ctx->regs = *frame;
}

cpu_context_t cpu_new_context(vaddr_t ip, vaddr_t rsp, bool user)
{
    cpu_context_t ret;

    memset(&ret, 0, sizeof(ret));

    ret.regs.rsp = rsp;
    ret.regs.rip = ip;
    ret.regs.rflags = 0x202;
    ret.regs.ss = user ? 0x3b : 0x30;
    ret.regs.cs = user ? 0x43 : 0x28;

    return ret;
}
