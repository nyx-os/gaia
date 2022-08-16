/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include "gdt.h"
#include <gaia/base.h>
#include <gaia/pmm.h>
#include <limine.h>
#include <stdc-shim/string.h>

void host_out8(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

void host_out16(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

void host_out32(uint16_t port, uint32_t value)
{
    __asm__ volatile("outl %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

uint8_t host_in8(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0"
                     : "=a"(value)
                     : "Nd"(port));
    return value;
}

uint16_t host_in16(uint16_t port)
{
    uint16_t value;
    __asm__ volatile("inw %1, %0"
                     : "=a"(value)
                     : "Nd"(port));
    return value;
}

uint32_t host_in32(uint16_t port)
{
    uint32_t value;
    __asm__ volatile("inl %1, %0"
                     : "=a"(value)
                     : "Nd"(port));
    return value;
}

void host_disable_interrupts(void)
{
    __asm__ volatile("cli");
}

void host_enable_interrupts(void)
{
    __asm__ volatile("sti");
}

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0};

void host_debug_write_string(const char *str)
{
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1)
    {
        return;
    }
    struct limine_terminal *terminal = terminal_request.response->terminals[0];
    terminal_request.response->write(terminal, str, strlen(str));
    while (*str)
    {
        host_out8(0xE9, *str);
        str++;
    }
}

uintptr_t host_phys_to_virt(uintptr_t phys)
{
    return phys + MMAP_IO_BASE;
}

uintptr_t host_virt_to_phys(uintptr_t virt)
{
    return virt - MMAP_IO_BASE;
}

const char *host_get_name(void)
{
    return "x86_64";
}

void host_initialize(void)
{
    gdt_initialize();
}
