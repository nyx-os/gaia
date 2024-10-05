#include "kernel/sched.hpp"
#include <amd64/asm.hpp>
#include <amd64/limine.h>
#include <hal/cpu.hpp>
#include <kernel/cpu.hpp>

namespace Gaia::Amd64 {

static void idlefn() { Hal::halt(); }

void cpu_init(Cpu *cpu) {
  (void)cpu;
  (void)idlefn;
}

} // namespace Gaia::Amd64

namespace Gaia::Hal {

Thread *get_current_thread() { return (Thread *)Amd64::get_gs_base(); };

void set_current_thread(Thread *thread) { Amd64::set_gs_base(thread); }

} // namespace Gaia::Hal
