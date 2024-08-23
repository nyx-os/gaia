/* SPDX-License-Identifier: BSD-2-Clause */
#include <hal/hal.hpp>

namespace Gaia::Hal {

// Serial output
void debug_output(char c) { *(uint8_t *)(0x10000000) = c; }

void halt() {
  for (;;) {
    asm volatile("wfi");
  }
}

Time get_time_since_boot() { return {0, 0, 0, 0, 0}; }

uintptr_t phys_to_virt(uintptr_t phys) { return phys + 0xffff800000000000; }
uintptr_t virt_to_phys(uintptr_t virt) { return virt - 0xffff800000000000; }

const size_t PAGE_SIZE = 4096;

void init_devices() {}

} // namespace Gaia::Hal
