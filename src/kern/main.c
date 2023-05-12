/* SPDX-License-Identifier: BSD-2-Clause */
#include "posix/fnctl.h"
#include <libkern/debug.h>
#include <kern/charon.h>
#include <kern/vm/vm.h>
#include <kern/vm/vm_kernel.h>
#include <dev/acpi.h>
#include <kern/sched.h>
#include <kern/term/term.h>
#include <posix/posix.h>

#ifndef __GAIA_GIT_VERSION__
#define __GAIA_GIT_VERSION__ "unknown"
#endif

void gaia_main(charon_t charon)
{
    machine_init();

    vm_init(charon);

    if (charon.rsdp != 0)
        acpi_init(charon);

    machine_init_devices();

    term_init(charon);

    log("gaia (%s)", __GAIA_GIT_VERSION__);
    log("Memory stats (pages): ");
    log("vm_kernel: %ld, phys: %ld, total: %ld",
        vm_kernel_stat().in_use / PAGE_SIZE, phys_used_pages(),
        phys_usable_pages());

    term_write("Welcome to the machine\n");

    sched_init();

    posix_init(charon);

    cpu_enable_interrupts();
}
