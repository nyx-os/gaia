/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef GAIA_FIRMWARE_LAPIC_H
#define GAIA_FIRMWARE_LAPIC_H
#include <gaia/base.h>

enum lapic_regs
{
    LAPIC_REG_ID = 0x20,
    LAPIC_REG_EOI = 0x0B0,
    LAPIC_REG_SPURIOUS_VECTOR = 0x0F0,
    LAPIC_REG_LVT_TIMER = 0x320,
    LAPIC_REG_TIMER_INIT_COUNT = 0x380,
    LAPIC_REG_TIMER_CURRENT_COUNT = 0x390,
    LAPIC_REG_TIMER_DIVIDE_CONFIG = 0x3E0,
};

enum apic_timer_division
{
    APIC_TIMER_DIVIDE_BY_2 = 0,
    APIC_TIMER_DIVIDE_BY_4 = 1,
    APIC_TIMER_DIVIDE_BY_8 = 2,
    APIC_TIMER_DIVIDE_BY_16 = 3,
    APIC_TIMER_DIVIDE_BY_32 = 4,
    APIC_TIMER_DIVIDE_BY_64 = 5,
    APIC_TIMER_DIVIDE_BY_128 = 6,
    APIC_TIMER_DIVIDE_BY_1 = 7
};

#define LAPIC_SPURIOUS_ALL 0xFF
#define LAPIC_SPURIOUS_ENABLE (1 << 8)
#define LAPIC_TIMER_IRQ 0x20
#define LAPIC_TIMER_PERIODIC (1 << 17)
#define LAPIC_TIMER_MASKED (1 << 16)

void lapic_initialize(void);
void lapic_eoi(void);

#endif /* GAIA_FIRMWARE_LAPIC_H */
