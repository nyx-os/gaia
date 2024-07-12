/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

#include <lib/base.hpp>
#include <lib/time.hpp>

namespace Gaia::Dev {
class AcpiPc;
}

namespace Gaia::Hal {

void debug_output(char c);
[[noreturn]] void halt();

Time get_time_since_boot();

void init_devices(Dev::AcpiPc *pc);

uintptr_t phys_to_virt(uintptr_t phys);
uintptr_t virt_to_phys(uintptr_t virt);

void disable_interrupts();
void enable_interrupts();

// TODO: figure out a way to make this constexpr
extern const size_t PAGE_SIZE;

struct CpuContext;

} // namespace Gaia::Hal
