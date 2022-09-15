/*
 * Copyright (c) 2022, lg.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "stdc-shim/string.h"
#include <gaia/base.h>
#include <gaia/firmware/acpi.h>
#include <gaia/firmware/lapic.h>
#include <gaia/host.h>
#include <gaia/pmm.h>
#include <gaia/sched.h>
#include <gaia/slab.h>
#include <gaia/syscall.h>
#include <gaia/term.h>
#include <gaia/vec.h>
#include <stdbool.h>

#define BOOTSTRAP_SERVER_NAME "/bootstrap"
#define BOOTSTRAP_SERVER_NAME_LENGTH 10

void gaia_main(Charon *charon)
{
    term_init(charon);

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

    sched_init();

    syscall_init(*charon);

    bool found = false;
    for (int i = 0; i < charon->modules.count; i++)
    {
        if (strncmp(charon->modules.modules[i].name, BOOTSTRAP_SERVER_NAME, BOOTSTRAP_SERVER_NAME_LENGTH) == 0)
        {
            sched_create_new_task_from_elf((uint8_t *)charon->modules.modules[i].address);
            found = true;
            break;
        }
    }

    if (!found)
    {
        panic("Cannot find bootstrap server");
    }

#ifdef DEBUG
    slab_dump();
#endif

    log("initial heap memory usage: %dkb", slab_used() / 1024);
    log("initial kernel memory usage: %dkb", pmm_get_allocated_pages() * PAGE_SIZE / 1024);
    log("gaia (0.0.1-proof-of-concept) finished booting on %s", host_get_name());
    log("Welcome to the machine!");

    host_enable_interrupts();
}
