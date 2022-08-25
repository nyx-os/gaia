/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/acpi.h>
#include <gaia/firmware/hpet.h>
#include <gaia/firmware/ioapic.h>
#include <gaia/firmware/lapic.h>
#include <gaia/firmware/madt.h>
#include <gaia/host.h>
#include <stdc-shim/string.h>

bool xsdt = false;

AcpiTableHeader *acpi_table_ptr;

void acpi_init(uintptr_t rsdp)
{
    Rsdp *rsdp_ptr = (Rsdp *)rsdp;

    log("ACPI revision number: %d", rsdp_ptr->revision);

    if (rsdp_ptr->revision)
    {
        acpi_table_ptr = (AcpiTableHeader *)(rsdp_ptr->xsdt);
        xsdt = true;
        log("XSDT is at %p", (uintptr_t)acpi_table_ptr);
    }
    else
    {
        acpi_table_ptr = (AcpiTableHeader *)((uintptr_t)rsdp_ptr->rsdt);
        log("RSDT is at %p", (uintptr_t)acpi_table_ptr);
    }

    madt_init();

    for (size_t i = 0; i < 16; i++) // set interrupt 32 to 48
    {
        ioapic_redirect_irq(0, i, i + 0x20);
    }

    hpet_initialize();
    lapic_initialize();
}

void *acpi_get_table(const char *signature)
{
    size_t entry_count = (acpi_table_ptr->length - sizeof(AcpiTableHeader)) / (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));
    for (size_t i = 0; i < entry_count; i++)
    {
        AcpiTableHeader *entry = (AcpiTableHeader *)host_phys_to_virt(!xsdt ? ((Rsdt *)acpi_table_ptr)->entry[i] : ((Xsdt *)acpi_table_ptr)->entry[i]);
        if (strncmp(entry->signature, signature, 4) == 0)
        {
            return entry;
        }
    }

    return NULL;
}

void acpi_dump_tables(void)
{
    size_t entry_count = (acpi_table_ptr->length - sizeof(AcpiTableHeader)) / (!xsdt ? sizeof(uint32_t) : sizeof(uint64_t));
    for (size_t i = 0; i < entry_count; i++)
    {
        AcpiTableHeader *entry = (AcpiTableHeader *)host_phys_to_virt(!xsdt ? ((Rsdt *)acpi_table_ptr)->entry[i] : ((Xsdt *)acpi_table_ptr)->entry[i]);
        char buf[5] = {0};
        memcpy(buf, entry->signature, 4);
        trace("Found system description table \"%s\"", buf);
    }
}
