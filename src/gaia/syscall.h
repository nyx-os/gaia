/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_SYSCALL_H
#define SRC_GAIA_SYSCALL_H

#include <gaia/host.h>

enum syscall_num
{
    SYSCALL_LOG = 0,
};

typedef struct
{
    uintptr_t first_arg;
    uintptr_t second_arg;
    uintptr_t third_arg;
    uintptr_t fourth_arg;
    uintptr_t fifth_arg;
} SyscallFrame;

void syscall(int num, SyscallFrame frame);

#endif /* SRC_GAIA_SYSCALL_H */
