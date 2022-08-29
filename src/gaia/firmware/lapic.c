/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/firmware/hpet.h>
#include <gaia/firmware/lapic.h>

#define LAPIC_BASE 0xFEE00000

static uint32_t lapic_read(uint32_t reg)
{
    return *(volatile uint32_t *)(host_phys_to_virt(LAPIC_BASE) + reg);
}

static void lapic_write(uint32_t reg, uint32_t value)
{
    *(volatile uint32_t *)(host_phys_to_virt(LAPIC_BASE) + reg) = value;
}

// lol
static uintptr_t get_lapic_address(void)
{
    uint32_t edx = 0, eax = 0;
    __asm__ volatile(
        "rdmsr\n\t"
        : "=a"(eax), "=d"(edx)
        : "c"(0x1b)
        : "memory");
    return (((uint64_t)edx << 32) | eax) & 0xffff00000;
}

void lapic_initialize(void)
{
    assert(get_lapic_address() == LAPIC_BASE);

    lapic_write(LAPIC_REG_SPURIOUS_VECTOR, lapic_read(LAPIC_REG_SPURIOUS_VECTOR) | LAPIC_SPURIOUS_ALL | LAPIC_SPURIOUS_ENABLE);

    lapic_write(LAPIC_REG_TIMER_DIVIDE_CONFIG, APIC_TIMER_DIVIDE_BY_16);

    lapic_write(LAPIC_REG_TIMER_INIT_COUNT, 0xffffffff);

    hpet_sleep(10);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED);

    uint32_t ticks_in_10ms = 0xffffffff - lapic_read(LAPIC_REG_TIMER_CURRENT_COUNT);

    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_IRQ | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_REG_TIMER_DIVIDE_CONFIG, APIC_TIMER_DIVIDE_BY_16);
    lapic_write(LAPIC_REG_TIMER_INIT_COUNT, ticks_in_10ms);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_REG_EOI, 0);
}
