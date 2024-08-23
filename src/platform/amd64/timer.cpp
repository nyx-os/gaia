/* SPDX-License-Identifier: BSD-2-Clause */
#include <amd64/asm.hpp>
#include <amd64/cpuid.hpp>
#include <amd64/hpet.hpp>
#include <amd64/timer.hpp>
#include <cstdint>
#include <dev/acpi/acpi.hpp>
#include <hal/hal.hpp>
#include <lib/log.hpp>

namespace Gaia::Amd64 {

uint64_t tsc_freq = 0;

void timer_init(Dev::AcpiPc *acpi) {

  // Reset TSC count
  wrmsr(0x10, 0);

  // Try with CPUID first as it is the most precise option
  auto cpuid_result = Cpuid::cpuid(0x15);

  // This *should* work in theory, but I don't have hardware to test it
  if (cpuid_result.is_ok()) {
    auto cpuid = cpuid_result.value().value();
    tsc_freq = (cpuid.ecx * cpuid.ebx) / cpuid.eax / 1000;
  }

  // Fallback to using timers instead
  else {

    hpet_init(acpi);

    // Use the HPET if present
    if (hpet_present()) {
      log("TSC calibration using HPET");

      auto init = rdtsc();

      hpet_sleep(10);

      tsc_freq = (rdtsc() - init) / 10;
    } else {
      panic("HPET is not present and CPU doesn't support CPUID leaf 0x16... "
            "get a new PC");
    }
  }

  uint64_t n = tsc_freq / 1000;

  log("CPU is running @ {}.{} GHz", (n / 1000), n % 1000);
}

uint64_t get_tsc_ms() {
  if (!tsc_freq)
    return 0;
  return rdtsc() / tsc_freq;
}

void timer_sleep(uint64_t ms) {
  if (hpet_present())
    hpet_sleep(ms);
}

} // namespace Gaia::Amd64
