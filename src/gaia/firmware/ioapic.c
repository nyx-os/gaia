/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/firmware/ioapic.h>
#include <gaia/firmware/madt.h>

// Use MMIO to interact with registers
static uint32_t ioapic_read(MadtIoApic *ioapic, uint32_t reg)
{
    uintptr_t base = host_phys_to_virt((uintptr_t)ioapic->address);
    *(volatile uint32_t *)(base) = reg;
    return *(volatile uint32_t *)(base + 0x10);
}

static void ioapic_write(MadtIoApic *ioapic, uint32_t reg, uint32_t value)
{
    uintptr_t base = host_phys_to_virt((uintptr_t)ioapic->address);
    *(volatile uint32_t *)(base) = reg;
    *(volatile uint32_t *)(base + 0x10) = value;
}

static size_t ioapic_get_max_redirect(MadtIoApic *ioapic)
{
    return (ioapic_read(ioapic, 1) & 0xff0000) >> 16;
}

static MadtIoApic *ioapic_from_gsi(uint32_t gsi)
{
    MadtIoApicVec ioapics = madt_get_ioapics();

    assert(ioapics.length > 0);

    for (size_t i = 0; i < ioapics.length; i++)
    {
        MadtIoApic ioapic = ioapics.data[i];

        // if the GSI is within the range of the ioapic, return the ioapic
        if (gsi >= ioapic.interrupt_base && gsi < ioapic.interrupt_base + ioapic_get_max_redirect(&ioapic))
        {
            return &ioapics.data[i];
        }
    }

    panic("Cannot find ioapic from gsi %d", gsi);
    return NULL;
}

static void ioapic_set_gsi_redirect(uint32_t lapic_id, uint8_t vector, uint8_t gsi, uint16_t flags)
{
    struct PACKED io_apic_redirect
    {
        uint8_t vector;
        uint8_t delivery_mode : 3;
        uint8_t dest_mode : 1;
        uint8_t delivery_status : 1;
        uint8_t polarity : 1;
        uint8_t remote_irr : 1;
        uint8_t trigger : 1;
        uint8_t mask : 1;
        uint8_t reserved : 7;
        uint8_t dest_id;
    };

    MadtIoApic *ioapic = ioapic_from_gsi(gsi);

    assert(ioapic != NULL);

    struct io_apic_redirect redirect = {0};
    redirect.vector = vector;

    if (flags & IOAPIC_ACTIVE_HIGH_LOW)
    {
        redirect.polarity = 1;
    }

    if (flags & IOAPIC_TRIGGER_EDGE_LOW)
    {
        redirect.trigger = 1;
    }

    redirect.dest_id = lapic_id;

    uint32_t io_redirect_table = (gsi - ioapic->interrupt_base) * 2 + 16;

    uint64_t redirect_u64 = *(uint64_t *)&redirect;

    ioapic_write(ioapic, io_redirect_table, (uint32_t)redirect_u64);
    ioapic_write(ioapic, io_redirect_table + 1, (uint32_t)(redirect_u64 >> 32));
}

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t irq, uint8_t vector)
{

    MadtISOVec isos = madt_get_isos();
    for (size_t i = 0; i < isos.length; i++)
    {
        MadtISO iso = isos.data[i];

        // if we find an iso entry for the irq, redirect it to the vector
        if (iso.irq_source == irq)
        {
            ioapic_set_gsi_redirect(lapic_id, vector, iso.global_system_interrupt, iso.flags);
            return;
        }
    }

    ioapic_set_gsi_redirect(lapic_id, vector, irq, 0);
}
