/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef GAIA_FIRMWARE_ACPI_H
#define GAIA_FIRMWARE_ACPI_H
#include <gaia/base.h>

typedef struct PACKED
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    char creator_id[4];
    uint32_t creator_revision;
} AcpiTableHeader;

typedef struct PACKED
{
    AcpiTableHeader header;
    uint32_t entry[];
} Rsdt;

typedef struct PACKED
{
    AcpiTableHeader header;
    uint64_t entry[];
} Xsdt;

typedef struct PACKED
{
    char sign[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt;
    uint32_t length;
    uint64_t xsdt;
    uint8_t ex_checksum;
    uint8_t reserved[3];
} Rsdp;

/**
 * @brief Initializes ACPI.
 * @param rsdp Address of the RSDP (see Charon).
 */
void acpi_init(uintptr_t rsdp);

/**
 * @brief Gets an ACPI system description table.
 *
 * @param signature The table's signature.
 * @return void* A pointer to the table.
 */
void *acpi_get_table(const char *signature);

/** @brief Dumps ACPI tables to debug output */
void acpi_dump_tables(void);

#endif /* GAIA_FIRMWARE_ACPI_H */
