/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <hal/cpu.hpp>
#include <hal/platform.hpp>
#include <kernel/sched.hpp>
#include <lib/spinlock.hpp>

namespace Gaia {

struct Cpu {
  Hal::CpuData data;
  uint32_t magic = 0xCAFEBABE;
  uint64_t ms;
  Thread *current_thread, *idle_thread, *previous_thread;
  Spinlock timer_lock;
};

Cpu *cpu_self();

} // namespace Gaia
