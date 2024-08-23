/* @license:bsd2 */
#pragma once
#include <hal/cpu.hpp>
#include <hal/platform.hpp>
#include <kernel/sched.hpp>

namespace Gaia {

struct Cpu {
  Hal::CpuData data;
  uint32_t magic = 0xCAFEBABE;
  uint64_t curr_time;
  Thread *current_thread, *idle_thread, *previous_thread;
};

Cpu *cpu_self();

} // namespace Gaia
