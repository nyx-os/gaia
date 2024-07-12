/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/string.hpp>
#include <kernel/sched.hpp>
#include <lib/base.hpp>

namespace Gaia {

Result<Void, Error> exec(Task &task, const char *path);

// Should probably be moved to POSIX
Result<Void, Error> execve(Task &task, const char *path, char const *argv[],
                           char const *envp[]);

static constexpr auto USER_STACK_TOP = 0x7fffffffe000;
static constexpr auto USER_STACK_SIZE =
    MIB(2); // This is allocated on-demand so we can make this very big

static constexpr auto KERNEL_STACK_SIZE = 0x1000 * 16;
} // namespace Gaia
