/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

#include <hal/platform.hpp>
#include <lib/base.hpp>
#include <lib/time.hpp>

namespace Gaia::Dev {
class AcpiPc;
}

namespace Gaia::Hal {

struct CpuData;

void debug_output(char c);
[[noreturn]] void halt();

Time get_time_since_boot();

void init_devices(Dev::AcpiPc *pc);

uintptr_t phys_to_virt(uintptr_t phys);
uintptr_t virt_to_phys(uintptr_t virt);

void disable_interrupts();
void enable_interrupts();

struct CpuContext;

} // namespace Gaia::Hal
