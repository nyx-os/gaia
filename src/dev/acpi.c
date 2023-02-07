/* SPDX-License-Identifier: BSD-2-Clause */
#include <dev/acpi.h>
#include <libkern/base.h>

static acpi_table_header_t *sdt;
static bool xsdt = false;

void acpi_init(charon_t charon)
{
    rsdp_t *rsdp = (rsdp_t *)charon.rsdp;

    if (rsdp->revision > 0) {
        sdt = (acpi_table_header_t *)P2V(rsdp->xsdt);
        xsdt = true;
    } else {
        sdt = (acpi_table_header_t *)P2V(rsdp->rsdt);
    }
}

void *acpi_get_table(const char *signature)
{
    size_t entry_count = (sdt->length - sizeof(*sdt)) /
                         (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));

    for (size_t i = 0; i < entry_count; i++) {
        acpi_table_header_t *entry = (acpi_table_header_t *)P2V(
                !xsdt ? ((rsdt_t *)sdt)->entry[i] : ((xsdt_t *)sdt)->entry[i]);

        if (strncmp(entry->signature, signature, 4) == 0) {
            return entry;
        }
    }

    return NULL;
}
