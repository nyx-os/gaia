/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "gaia/host.h"
#include "gaia/debug.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include <gaia/base.h>
#include <gaia/pmm.h>
#include <gaia/slab.h>
#include <gaia/term.h>
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

void host_debug_write_string(const char *str)
{
    term_write(str);
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

volatile static struct limine_hhdm_request hhdm = {
    .id = LIMINE_HHDM_REQUEST,
    0};

uintptr_t host_kern_to_phys(uintptr_t virt)
{
    return virt - hhdm.response->offset;
}

uintptr_t host_phys_to_kern(uintptr_t phys)
{
    return phys + hhdm.response->offset;
}

const char *host_get_name(void)
{
    return "x86_64";
}

void host_initialize(void)
{
    paging_initialize();
    gdt_initialize();
    idt_initialize();
}

void *host_allocate_page(void)
{
    return pmm_alloc_zero();
}

void host_free_page(void *ptr)
{
    pmm_free(ptr);
}

void host_load_pagemap(Pagemap *pagemap)
{
    paging_load_pagemap(pagemap);
}

void host_hang(void)
{
    while (true)
    {
        __asm__ volatile("hlt");
    }
}

void host_map_page(Pagemap *pagemap, uintptr_t vaddr, uintptr_t paddr, PageFlags flags)
{
    uint64_t flags_ = PTE_PRESENT;

    if (flags & PAGE_WRITABLE)
    {
        flags_ |= PTE_WRITABLE;
    }
    if (flags & PAGE_NOT_EXECUTABLE)
    {
        flags_ |= PTE_NOT_EXECUTABLE;
    }
    if (flags & PAGE_USER)
    {
        flags_ |= PTE_USER;
    }

    paging_map_page(pagemap, vaddr, paddr, flags_, (flags & PAGE_HUGE));
}

void host_unmap_page(Pagemap *pagemap, uintptr_t vaddr)
{
    paging_unmap_page(pagemap, vaddr);
}
