/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_SYSCALL_H
#define SRC_GAIA_SYSCALL_H

#include <gaia/charon.h>
#include <gaia/host.h>

enum syscall_num
{
    SYSCALL_LOG = 0,
    SYSCALL_ALLOC_PORT = 1,
    SYSCALL_SEND = 2,
    SYSCALL_RECV = 3,
};

typedef struct
{
    uintptr_t first_arg;
    uintptr_t second_arg;
    uintptr_t third_arg;
    uintptr_t fourth_arg;
    uintptr_t fifth_arg;
    uintptr_t *return_value;
    uintptr_t intstruction_pointer;
    InterruptStackframe *int_frame;
} SyscallFrame;

void syscall(int num, SyscallFrame frame);
void syscall_init(Charon charon);

#endif /* SRC_GAIA_SYSCALL_H */
