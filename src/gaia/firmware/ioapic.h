/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef GAIA_FIRMWARE_IOAPIC_H
#define GAIA_FIRMWARE_IOAPIC_H
#include <gaia/base.h>

enum IoApicReg
{
    IOAPIC_REG_VERSION = 0x01,
};

#define IOAPIC_ACTIVE_HIGH_LOW (1 << 1)
#define IOAPIC_TRIGGER_EDGE_LOW (1 << 3)

void ioapic_redirect_irq(uint32_t lapic_id, uint8_t irq, uint8_t vector);

#endif /* GAIA_FIRMWARE_IOAPIC_H */
