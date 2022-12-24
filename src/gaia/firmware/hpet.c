/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/hpet.h>
#include <gaia/host.h>

static uint64_t hpet_base = 0;
static uint64_t clock = 0;

static uint64_t hpet_read(uint64_t reg)
{
    return *(uint64_t *)(hpet_base + reg);
}

static void hpet_write(uint64_t reg, uint64_t val)
{
    *(uint64_t *)(hpet_base + reg) = val;
}

void hpet_initialize(void)
{
    Hpet *hpet = (Hpet *)acpi_get_table("HPET");

    if (!hpet)
    {
        panic("HPET not found, get a new PC");
    }

    hpet_base = host_phys_to_virt(hpet->address);

    // We don't support system io.
    assert(hpet->address_space_id == HPET_ADDRESS_SPACE_MEMORY);

    // get the last 32 bits
    clock = hpet_read(HPET_GENERAL_CAPABILITIES) >> HPET_CAP_COUNTER_CLOCK_OFFSET;

    assert(clock != 0);
    assert(clock <= 0x05F5E100);

    hpet_write(HPET_GENERAL_CONFIGURATION, HPET_CONFIG_DISABLE);
    hpet_write(HPET_MAIN_COUNTER_VALUE, 0);
    hpet_write(HPET_GENERAL_CONFIGURATION, HPET_CONFIG_ENABLE);
}

void hpet_sleep(int ms)
{
    uint64_t target = hpet_read(HPET_MAIN_COUNTER_VALUE) + (ms * 1000000000000) / clock;
    while (hpet_read(HPET_MAIN_COUNTER_VALUE) < target)
    {
        __asm__ volatile("pause");
    }
}
