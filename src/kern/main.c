/* SPDX-License-Identifier: BSD-2-Clause */
#include <libkern/debug.h>
#include <kern/charon.h>
#include <kern/vm/vm.h>

void gaia_main(charon_t charon)
{
    vm_init(charon);
    machine_init();

    log("gaia version 0.0.1");
}
