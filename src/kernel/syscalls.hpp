/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include "x86_64/idt.hpp"
#include <cstdint>
#include <lib/base.hpp>

namespace Gaia {

struct SyscallParams {
  uint64_t param1, param2, param3, param4, param5, param6, origin;
  Hal::InterruptFrame *frame;
};

uint64_t syscall(int num, SyscallParams params);

} // namespace Gaia
