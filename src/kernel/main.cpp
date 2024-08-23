/* SPDX-License-Identifier: BSD-2-Clause */
#include <dev/acpi/acpi.hpp>
#include <dev/builtin.hpp>
#include <dev/console/fbconsole.hpp>
#include <dev/devkit/registry.hpp>
#include <fs/tmpfs.hpp>
#include <fs/vfs.hpp>
#include <hal/hal.hpp>
#include <kernel/elf.hpp>
#include <kernel/main.hpp>
#include <kernel/sched.hpp>
#include <kernel/timer.hpp>
#include <lib/list.hpp>
#include <posix/errno.hpp>
#include <posix/exec.hpp>
#include <posix/fd.hpp>
#include <vm/phys.hpp>
#include <vm/vm.hpp>

using namespace Gaia;

Result<Void, Error> Gaia::main(Charon charon) {
  static Dev::FbConsole fbconsole(charon);

  Vm::phys_init(charon);
  Vm::init();

  Dev::create_registry();
  Dev::initialize_kernel_drivers();
  Dev::AcpiPc pc(charon);

  pc.dump_tables();

  /*
   * We want to initialize the scheduler before everything else, so we can use
   * cpu_self() and other goodies like events
   */
  TRY(sched_init());

  Hal::init_devices(&pc);

  pc.load_drivers();

  Fs::tmpfs_init(charon);

  Dev::system_console()->create_dev();

  auto timer = new Timer;
  timer->set(nullptr, 100);
  timer_enqueue(timer);

  auto timer2 = new Timer;
  timer2->set(nullptr, 250);
  timer_enqueue(timer2);

  auto task =
      TRY(sched_new_task(sched_allocate_pid(), sched_kernel_task(), true));

  const char *argv[] = {"/usr/bin/init", nullptr};
  const char *envp[] = {"SHELL=/usr/bin/bash", nullptr};

  log("Launching init program /usr/bin/init");
  TRY(execve(*task, "/usr/bin/init", argv, envp));

  log("Gaia v0.0.1 (git version {}), {} pages are available",
      __GAIA_GIT_VERSION__, Vm::phys_usable_pages());

  Dev::system_console()->clear();
  Hal::enable_interrupts();

  return Ok({});
}
