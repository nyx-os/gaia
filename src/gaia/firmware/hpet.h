/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef GAIA_FIRMWARE_HPET_H
#define GAIA_FIRMWARE_HPET_H
#include <gaia/firmware/acpi.h>

#define HPET_ADDRESS_SPACE_MEMORY 0
#define HPET_ADDRESS_SPACE_IO 1

#define HPET_CAP_COUNTER_CLOCK_OFFSET (32)

#define HPET_CONFIG_ENABLE (1)
#define HPET_CONFIG_DISABLE (0)

typedef struct PACKED
{
    AcpiTableHeader header;
    uint8_t hardware_rev_id;
    uint8_t info;
    uint16_t pci_vendor_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved1;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} Hpet;

enum hpet_registers
{
    HPET_GENERAL_CAPABILITIES = 0,
    HPET_GENERAL_CONFIGURATION = 16,
    HPET_MAIN_COUNTER_VALUE = 240,
};

void hpet_initialize(void);
void hpet_sleep(int ms);

#endif /* GAIA_FIRMWARE_HPET_H */
