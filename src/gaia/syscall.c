/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/base.h>
#include <gaia/host.h>
#include <gaia/syscall.h>

static void sys_log(SyscallFrame frame)
{
    log("%s", (char *)frame.first_arg);
}

static void (*syscall_table[1])(SyscallFrame) = {
    [0] = sys_log,
};

void syscall(int num, SyscallFrame frame)
{
    syscall_table[num](frame);
}
