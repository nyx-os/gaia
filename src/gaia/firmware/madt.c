/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/madt.h>
#include <gaia/vec.h>

Vec(MadtLapic) madt_lapics = {0};
static MadtIoApicVec madt_ioapics = {0};
static MadtISOVec madt_isos = {0};

void madt_init(void)
{
    Madt *madt = (Madt *)acpi_get_table("APIC");
    assert(madt != NULL);

    size_t i = 0;
    size_t cpu_count = 0;
    while (i < madt->header.length - sizeof(Madt))
    {
        MadtEntryHeader *header = (MadtEntryHeader *)(madt->entries + i);

        switch (header->type)
        {
        case 0:
        {
            MadtLapic lapic = *(MadtLapic *)(header);

            if (lapic.flags & LAPIC_ENABLED)
            {
                //    vec_push(&madt_lapics, lapic);
                cpu_count++;
            }

            else if (lapic.flags & LAPIC_ONLINE_BOOTABLE)
            {
                cpu_count++;
            }

            DISCARD(madt_lapics);

            break;
        }
        case 1:
            vec_push(&madt_ioapics, *(MadtIoApic *)(madt->entries + i));
            break;
        case 2:
            vec_push(&madt_isos, *(MadtISO *)(madt->entries + i));
            break;
        }

        i += MAX(2, header->length);
    }

    log("CPU has %d %s", cpu_count, cpu_count == 1 ? "core" : "cores");
}

MadtIoApicVec madt_get_ioapics(void)
{
    return madt_ioapics;
}

MadtISOVec madt_get_isos(void)
{
    return madt_isos;
}
