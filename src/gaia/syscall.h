/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef SRC_GAIA_SYSCALL_H
#define SRC_GAIA_SYSCALL_H

#include <gaia/charon.h>
#include <gaia/host.h>

#define GAIA_SYS_LOG 0
#define GAIA_SYS_EXIT 1
#define GAIA_SYS_MSG 2
#define GAIA_SYS_ALLOC_PORT 3
#define GAIA_SYS_GET_PORT 4
#define GAIA_SYS_REGISTER_PORT 5
#define GAIA_SYS_SPAWN 6

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
