/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/acpi.h>
#include <gaia/host.h>
#include <stdc-shim/string.h>

bool xsdt = false;

TableHeader *acpi_table_ptr;

void acpi_init(uintptr_t rsdp)
{
    Rsdp *rsdp_ptr = (Rsdp *)rsdp;

    log("ACPI revision number: %d", rsdp_ptr->revision);

    if (rsdp_ptr->revision)
    {
        acpi_table_ptr = (TableHeader *)(rsdp_ptr->xsdt);
        xsdt = true;
        log("XSDT is at %p", (uintptr_t)acpi_table_ptr);
    }
    else
    {
        acpi_table_ptr = (TableHeader *)((uintptr_t)rsdp_ptr->rsdt);
        log("RSDT is at %p", (uintptr_t)acpi_table_ptr);
    }
}

void *acpi_get_table(const char *signature)
{
    size_t entry_count = (acpi_table_ptr->length - sizeof(TableHeader)) / (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));
    for (size_t i = 0; i < entry_count; i++)
    {
        TableHeader *entry = (TableHeader *)host_phys_to_virt(!xsdt ? ((Rsdt *)acpi_table_ptr)->entry[i] : ((Xsdt *)acpi_table_ptr)->entry[i]);
        if (strncmp(entry->signature, signature, 4) == 0)
        {
            return entry;
        }
    }

    return NULL;
}

void acpi_dump_tables(void)
{
    size_t entry_count = (acpi_table_ptr->length - sizeof(TableHeader)) / (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));
    for (size_t i = 0; i < entry_count; i++)
    {
        TableHeader *entry = (TableHeader *)host_phys_to_virt(!xsdt ? ((Rsdt *)acpi_table_ptr)->entry[i] : ((Xsdt *)acpi_table_ptr)->entry[i]);
        char buf[5] = {0};
        memcpy(buf, entry->signature, 4);
        trace("Found system description table \"%s\"", buf);
    }
}
