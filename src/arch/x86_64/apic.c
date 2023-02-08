/* SPDX-License-Identifier: BSD-2-Clause */
#include <libkern/base.h>
#include <dev/acpi.h>
#include <x86_64/apic.h>
#include <x86_64/madt.h>
#include <x86_64/asm.h>

void hpet_init(void);
void hpet_sleep(int ms);

typedef struct ioapics_entry {
    madt_ioapic_t ioapic;
    TAILQ_ENTRY(ioapics_entry) entry;
} ioapics_entry_t;

typedef struct isos_entry {
    madt_iso_t iso;
    TAILQ_ENTRY(isos_entry) entry;
} isos_entry_t;

static TAILQ_HEAD(ioapics_head, ioapics_entry) ioapics;
static TAILQ_HEAD(isos_head, isos_entry) isos;
static uintptr_t lapic_base = 0;

static void madt_init(void)
{
    madt_t *madt;

    TAILQ_INIT(&ioapics);
    TAILQ_INIT(&isos);

    madt = acpi_get_table("APIC");

    assert(madt != NULL);

    size_t i = 0;
    while (i < madt->header.length - sizeof(madt_t)) {
        madt_entry_header_t *header =
                (madt_entry_header_t *)(madt->entries + i);

        switch (header->type) {
        case 1: {
            ioapics_entry_t *new_ent = kmalloc(sizeof(ioapics_entry_t));
            new_ent->ioapic = *(madt_ioapic_t *)(header);
            TAILQ_INSERT_TAIL(&ioapics, new_ent, entry);
            break;
        }

        case 2: {
            isos_entry_t *new_ent = kmalloc(sizeof(isos_entry_t));
            new_ent->iso = *(madt_iso_t *)(header);
            TAILQ_INSERT_TAIL(&isos, new_ent, entry);
            break;
        }
        }

        i += MAX(2, header->length);
    }
}

static uint32_t ioapic_read(madt_ioapic_t ioapic, uint32_t reg)
{
    uintptr_t base = P2V(ioapic.address);

    VOLATILE_WRITE(uint32_t, base, reg);

    return VOLATILE_READ(uint32_t, base + 0x10);
}

static void ioapic_write(madt_ioapic_t ioapic, uint32_t reg, uint32_t value)
{
    uintptr_t base = P2V(ioapic.address);

    VOLATILE_WRITE(uint32_t, base, reg);
    VOLATILE_WRITE(uint32_t, base + 0x10, value);
}

static size_t ioapic_get_max_redirect(madt_ioapic_t ioapic)
{
    struct PACKED ioapic_version {
        uint8_t version;
        uint8_t reserved;
        uint8_t max_redirect;
        uint8_t reserved2;
    };

    uint32_t val = ioapic_read(ioapic, 1);

    struct ioapic_version version = *(struct ioapic_version *)&val;

    return version.max_redirect;
}

static madt_ioapic_t ioapic_from_gsi(uint32_t gsi)
{
    ioapics_entry_t *ent;
    TAILQ_FOREACH(ent, &ioapics, entry)
    {
        /* if the GSI is within the range of the ioapic, return the ioapic */
        if (gsi >= ent->ioapic.interrupt_base &&
            gsi < ent->ioapic.interrupt_base +
                            ioapic_get_max_redirect(ent->ioapic)) {
            return ent->ioapic;
        }
    }

    panic("Cannot find ioapic from gsi %d", gsi);
    return (madt_ioapic_t){ { -1, -1 }, -1, -1, -1, -1 };
}

static void ioapic_set_gsi_redirect(uint32_t lapic_id, uint8_t vector,
                                    uint8_t gsi, uint16_t flags)
{
    typedef union PACKED {
        struct PACKED {
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
        } _redirect;

        struct PACKED {
            uint32_t _low_byte;
            uint32_t _high_byte;
        } _raw;
    } ioapic_redirect_t;

    madt_ioapic_t ioapic = ioapic_from_gsi(gsi);

    ioapic_redirect_t redirect = { 0 };
    redirect._redirect.vector = vector;

    if (flags & IOAPIC_ACTIVE_HIGH_LOW) {
        redirect._redirect.polarity = 1;
    }

    if (flags & IOAPIC_TRIGGER_EDGE_LOW) {
        redirect._redirect.trigger = 1;
    }

    redirect._redirect.dest_id = lapic_id;

    uint32_t io_redirect_table = (gsi - ioapic.interrupt_base) * 2 + 16;

    ioapic_write(ioapic, io_redirect_table, (uint32_t)redirect._raw._low_byte);
    ioapic_write(ioapic, io_redirect_table + 1, redirect._raw._high_byte);
}

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t irq, uint8_t vector)
{
    isos_entry_t *ent;
    int count = 0;

    TAILQ_FOREACH(ent, &isos, entry)
    {
        /* if we find an iso entry for the irq, redirect it to the vector */
        if (ent->iso.irq_source == irq) {
            ioapic_set_gsi_redirect(lapic_id, vector, ent->iso.gsi,
                                    ent->iso.flags);
            return;
        }

        if (count > 255) {
            panic("Couldn't find ISO entry for irq %d", irq);
        }

        count++;
    }

    ioapic_set_gsi_redirect(lapic_id, vector, irq, 0);
}

static uint32_t lapic_read(uint32_t reg)
{
    return VOLATILE_READ(uint32_t, P2V(lapic_base) + reg);
}

static void lapic_write(uint32_t reg, uint32_t value)
{
    VOLATILE_WRITE(uint32_t, P2V(lapic_base) + reg, value);
}

static uintptr_t get_lapic_address(void)
{
    return rdmsr(0x1b) & 0xfffff000;
}

void lapic_init(void)
{
    lapic_base = get_lapic_address();

    lapic_write(LAPIC_REG_SPURIOUS_VECTOR,
                lapic_read(LAPIC_REG_SPURIOUS_VECTOR) | LAPIC_SPURIOUS_ALL |
                        LAPIC_SPURIOUS_ENABLE);

    lapic_write(LAPIC_REG_TIMER_DIVIDE_CONFIG, APIC_TIMER_DIVIDE_BY_16);

    lapic_write(LAPIC_REG_TIMER_INIT_COUNT, -1);

    hpet_sleep(10);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED);

    uint32_t ticks_in_10ms = -1 - lapic_read(LAPIC_REG_TIMER_CURRENT_COUNT);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_IRQ | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_REG_TIMER_DIVIDE_CONFIG, APIC_TIMER_DIVIDE_BY_16);
    lapic_write(LAPIC_REG_TIMER_INIT_COUNT, ticks_in_10ms / 10);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_REG_EOI, 0);
}

void apic_init(void)
{
    madt_init();

    for (int i = 0; i < 16; i++) {
        ioapic_redirect_irq(0, i, i + 32);
    }

    hpet_init();
    lapic_init();
}
