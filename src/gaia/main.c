/*
 * Copyright (c) 2022, lg.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include <gaia/base.h>
#include <gaia/firmware/acpi.h>
#include <gaia/firmware/lapic.h>
#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/vec.h>
#include <stdbool.h>

void gaia_main(Charon *charon)
{
    pmm_init(*charon);
#ifdef DEBUG
    pmm_dump();
#endif

    slab_init();

    acpi_init(charon->rsdp);
#ifdef DEBUG
    acpi_dump_tables();
#endif
    host_initialize();

#ifdef DEBUG
    slab_dump();
#endif

    // host_enable_interrupts();
    log("initial kernel memory usage: %dkb", pmm_get_allocated_pages() * PAGE_SIZE / 1024);
    log("initial heap memory usage: %dkb", slab_used() / 1024);
    log("gaia (0.0.1-proof-of-concept) finished booting on %s", host_get_name());
    log("Welcome to the machine!");
}
