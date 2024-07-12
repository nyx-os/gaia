/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <x86_64/idt.hpp>

namespace Gaia::x86_64 {
extern "C" void syscall_entry(Hal::InterruptFrame *frame);
void syscall_init();
} // namespace Gaia::x86_64
