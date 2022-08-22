/*
 * Copyright (c) 2022, lg.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/base.h>
#include <gaia/charon.h>
#include <gaia/firmware/acpi.h>
#include <gaia/pmm.h>
#include <stdbool.h>

void gaia_main(Charon *charon)
{
    pmm_init(*charon);
    pmm_dump();

    host_initialize();

    log("gaia (0.0.1-proof-of-concept) finished booting on %s", host_get_name());
    log("Welcome to the machine!");
}
