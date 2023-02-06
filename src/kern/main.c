/* SPDX-License-Identifier: BSD-2-Clause */
#include <libkern/debug.h>
#include <kern/charon.h>
#include <kern/vm/vm.h>

void gaia_main(charon_t charon)
{
    machine_init();
    vm_init(charon);

    log("used pages: %lx", phys_used_pages());
}
