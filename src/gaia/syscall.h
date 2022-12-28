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
#define GAIA_SYS_VM_CREATE 6
#define GAIA_SYS_CREATE_TASK 7
#define GAIA_SYS_START_TASK 8
#define GAIA_SYS_VM_WRITE 9
#define GAIA_SYS_VM_MAP 10
#define GAIA_SYS_VM_REGISTER 11
#define GAIA_SYS_FREE_PORT 12
#define GAIA_SYS_YIELD 13
#define GAIA_SYS_GETPID 14
#define GAIA_SYS_GET_TASK 15
#define GAIA_SYS_SET_FS_BASE 16

typedef struct
{
    uintptr_t first_arg;
    uintptr_t second_arg;
    uintptr_t third_arg;
    uintptr_t fourth_arg;
    uintptr_t fifth_arg;
    uintptr_t *return_value, *errno;
    uintptr_t instruction_pointer;
    InterruptStackframe *int_frame;
} SyscallFrame;

void syscall(int num, SyscallFrame frame);
void syscall_init(Charon charon);

#endif /* SRC_GAIA_SYSCALL_H */
