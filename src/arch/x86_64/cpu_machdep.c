/* SPDX-License-Identifier: BSD-2-Clause */
#include "kern/vm/phys.h"
#include <machdep/cpu.h>
#include <x86_64/asm.h>

void cpu_halt(void)
{
    while (true) {
        hlt();
    }
}

void simd_save_state(void *state);
void simd_restore_state(void *state);
void simd_init_context(void *state);

void cpu_enable_interrupts(void)
{
    sti();
}

void cpu_switch_context(intr_frame_t *frame, cpu_context_t ctx)
{
    simd_restore_state(ctx.fpu_context);
    *frame = ctx.regs;
}

void cpu_save_context(intr_frame_t *frame, cpu_context_t *ctx)
{
    simd_save_state(ctx->fpu_context);
    ctx->regs = *frame;
}

cpu_context_t cpu_new_context(vaddr_t ip, vaddr_t rsp, bool user)
{
    cpu_context_t ret;

    memset(&ret, 0, sizeof(ret));

    ret.fpu_context = (void *)P2V(phys_allocz());
    simd_init_context(ret.fpu_context);
    ret.regs.rsp = rsp;
    ret.regs.rip = ip;
    ret.regs.rflags = 0x202;
    ret.regs.ss = user ? 0x3b : 0x30;
    ret.regs.cs = user ? 0x43 : 0x28;

    return ret;
}
