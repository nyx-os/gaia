/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef GAIA_FIRMWARE_MADT_H
#define GAIA_FIRMWARE_MADT_H
#include <gaia/base.h>
#include <gaia/firmware/acpi.h>
#include <gaia/vec.h>

typedef struct PACKED
{
    AcpiTableHeader header;
    uint32_t local_interrupt_controller;
    uint32_t flags;
    uint8_t entries[];
} Madt;

typedef struct PACKED
{
    uint8_t type, length;
} MadtEntryHeader;

typedef struct PACKED
{
    MadtEntryHeader header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} MadtLapic;

typedef struct PACKED
{
    MadtEntryHeader header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t interrupt_base;
} MadtIoApic;

// Interrupt Source Override
typedef struct PACKED
{
    MadtEntryHeader header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} MadtISO;

typedef struct PACKED
{
    MadtEntryHeader header;
    uint8_t apic_id;
    uint16_t flags;
    uint32_t lint;
} LapicNmi;

enum LapicFlags
{
    LAPIC_ENABLED = 1 << 0,
    LAPIC_ONLINE_BOOTABLE = 1 << 1,
};

void madt_init(void);

typedef Vec(MadtIoApic) MadtIoApicVec;
typedef Vec(MadtISO) MadtISOVec;

MadtIoApicVec madt_get_ioapics(void);
MadtISOVec madt_get_isos(void);

#endif /* GAIA_FIRMWARE_MADT_H */
