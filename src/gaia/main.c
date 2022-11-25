/*
 * Copyright (c) 2022, lg.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/rights.h"
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

static Charon *_charon;

Charon *gaia_get_charon(void)
{
    return _charon;
}

void gaia_main(Charon *charon)
{
    term_init(charon);

    pmm_init(*charon);
#ifdef DEBUG
    pmm_dump();
#endif

    slab_init();

    _charon = (void *)host_phys_to_virt((uintptr_t)pmm_alloc_zero());

    memcpy(_charon, charon, sizeof(Charon));


    for(size_t i = 0; i < _charon->modules.count; i++)
    {
        CharonModule module = _charon->modules.modules[i];
        log("%s: %p", module.name, module.address);
    }

    _charon->framebuffer.address = host_virt_to_phys(_charon->framebuffer.address);

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
            sched_create_new_task_from_elf((uint8_t *)(host_phys_to_virt(charon->modules.modules[i].address)), RIGHT_DMA);
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
