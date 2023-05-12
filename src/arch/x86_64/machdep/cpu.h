/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_MACHDEP_CPU_H_
#define SRC_ARCH_X86_64_MACHDEP_CPU_H_
#include <machdep/intr.h>
#include <machdep/vm.h>
#include <stdbool.h>

#define USER_STACK_TOP 0x7fffffffe000

typedef struct {
    intr_frame_t regs;
    void *fpu_context;
} cpu_context_t;

void cpu_halt(void);
void cpu_enable_interrupts(void);
void cpu_switch_context(intr_frame_t *frame, cpu_context_t ctx);
void cpu_save_context(intr_frame_t *frame, cpu_context_t *ctx);

cpu_context_t cpu_new_context(vaddr_t ip, vaddr_t rsp, bool user);

#endif
