/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <amd64/idt.hpp>

namespace Gaia::Amd64 {
extern "C" void syscall_entry(Hal::InterruptFrame *frame);
void syscall_init();
} // namespace Gaia::Amd64
