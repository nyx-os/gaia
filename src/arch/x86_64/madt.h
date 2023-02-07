/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_ARCH_X86_64_MADT_H_
#define SRC_ARCH_X86_64_MADT_H_
#include <dev/acpi.h>
#include <libkern/base.h>

typedef struct PACKED {
    acpi_table_header_t header;
    uint32_t local_interrupt_controller;
    uint32_t flags;
    uint8_t entries[];
} madt_t;

typedef struct PACKED {
    uint8_t type, length;
} madt_entry_header_t;

typedef struct PACKED {
    madt_entry_header_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags;
} madt_lapic_t;

typedef struct PACKED {
    madt_entry_header_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t interrupt_base;
} madt_ioapic_t;

typedef struct PACKED {
    madt_entry_header_t header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
} madt_iso_t;

#endif
