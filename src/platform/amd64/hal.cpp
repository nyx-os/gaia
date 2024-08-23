/* SPDX-License-Identifier: BSD-2-Clause */
#include <amd64/apic.hpp>
#include <amd64/asm.hpp>
#include <amd64/cpuid.hpp>
#include <amd64/gdt.hpp>
#include <amd64/syscall.hpp>
#include <amd64/timer.hpp>
#include <dev/acpi/acpi.hpp>
#include <dev/console/fbconsole.hpp>
#include <hal/hal.hpp>
#include <hal/int.hpp>

using namespace Gaia::Amd64;

namespace Gaia::Amd64 {
void cpu_init(Cpu *cpu);
uintptr_t get_tsc_ms();
} // namespace Gaia::Amd64

namespace Gaia::Hal {
void debug_output(char c) {
  outb(0xe9, c);
  if (Dev::system_console()) {
    Dev::system_console()->log_output(c);
  }
}

[[noreturn]] void halt() {
  for (;;) {
    hlt();
  }
}

void disable_interrupts() { cli(); }
void enable_interrupts() { sti(); }

Time get_time_since_boot() {
  auto ms = get_tsc_ms();

  uint64_t seconds = ms / 1000;
  uint64_t left = ms % 1000;

  return {0, 0, seconds, left, 0};
}

uintptr_t phys_to_virt(uintptr_t phys) { return phys + 0xffff800000000000; }
uintptr_t virt_to_phys(uintptr_t virt) { return virt - 0xffff800000000000; }

static Cpu _cpu;

void init_devices(Dev::AcpiPc *pc) {
  Amd64::cpu_init(&_cpu);
  Amd64::timer_init(pc);
  Amd64::simd_init();
  log("CPU is {}", Cpuid::branding().brand);
  Amd64::ioapic_init(pc);
  Amd64::gdt_init_tss();
  Amd64::syscall_init();
}

Ipl get_ipl() { return (Ipl)read_cr8(); }

void set_ipl(Ipl ipl) { write_cr8((uint64_t)ipl); }

void set_timer(uint64_t ns) { (void)ns; }

} // namespace Gaia::Hal
