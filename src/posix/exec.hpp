/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/string.hpp>
#include <kernel/sched.hpp>
#include <lib/base.hpp>

namespace Gaia::Posix {

Result<Void, Error> execve(Task *task, frg::string_view path,
                           char const *argv[], char const *envp[]);

} // namespace Gaia::Posix
