/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_MACHDEP_INTR_H_
#define SRC_ARCH_X86_64_MACHDEP_INTR_H_
#include <stddef.h>
#include <stdint.h>

typedef struct PACKED {
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
} intr_frame_t;

#define SYSCALL_INT 0x42

typedef void (*intr_handler_t)(intr_frame_t *frame);

void intr_register(int vec, intr_handler_t handler);

#endif
