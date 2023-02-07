/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef SRC_DEV_ACPI_H_
#define SRC_DEV_ACPI_H_
#include <libkern/base.h>
#include <kern/charon.h>

typedef struct PACKED {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    char creator_id[4];
    uint32_t creator_revision;
} acpi_table_header_t;

typedef struct PACKED {
    acpi_table_header_t header;
    uint32_t entry[];
} rsdt_t;

typedef struct PACKED {
    acpi_table_header_t header;
    uint64_t entry[];
} xsdt_t;

typedef struct PACKED {
    char sign[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt;
    uint32_t length;
    uint64_t xsdt;
    uint8_t ex_checksum;
    uint8_t reserved[3];
} rsdp_t;

void acpi_init(charon_t charon);
void *acpi_get_table(const char *name);

#endif
