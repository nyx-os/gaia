/*
 * Copyright (c) 2022, lg.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/base.h>
#include <gaia/charon.h>
#include <gaia/limine.h>
#include <stdc-shim/string.h>
#include <stddef.h>
#include <stdint.h>

static void done(void)
{
    for (;;)
    {
        __asm__("hlt");
    }
}

// TODO: Maybe move this to a header file?
Charon limine_to_charon();

static Charon charon = {0};
void gaia_main(Charon *charon);

void _start(void)
{
    charon = limine_to_charon();
    gaia_main(&charon);
    done();
}
