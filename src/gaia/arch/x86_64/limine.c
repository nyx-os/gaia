/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <gaia/charon.h>
#include <limine.h>

volatile static struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0};

volatile static struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0};

volatile static struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0};

volatile static struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0};

static CharonFramebuffer limine_fb_to_charon(struct limine_framebuffer *fb)
{
    return (CharonFramebuffer){
        .address = (uintptr_t)fb->address,
        .width = fb->width,
        .height = fb->height,
        .pitch = fb->pitch,
        .bpp = fb->bpp,
        .present = true};
}

static CharonMemoryMapEntryType limine_mmap_type_to_charon(uint64_t type)
{
    switch (type)
    {
    case LIMINE_MEMMAP_USABLE:
        return MMAP_FREE;
    case LIMINE_MEMMAP_RESERVED:
        return MMAP_RESERVED;
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return MMAP_RECLAIMABLE;
    case LIMINE_MEMMAP_ACPI_NVS:
        return MMAP_RESERVED;
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return MMAP_RECLAIMABLE;
    case LIMINE_MEMMAP_KERNEL_AND_MODULES:
        return MMAP_MODULE;
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return MMAP_FRAMEBUFFER;
    default:
        return MMAP_RESERVED;
    }
}

static CharonMemoryMap limine_mmap_to_charon(struct limine_memmap_response *mmap)
{
    CharonMemoryMap ret;

    ret.count = mmap->entry_count;

    for (size_t i = 0; i < ret.count; i++)
    {
        ret.entries[i] = (CharonMemoryMapEntry){
            .type = limine_mmap_type_to_charon(mmap->entries[i]->type),
            .base = mmap->entries[i]->base,
            .size = mmap->entries[i]->length};
    }

    return ret;
}

static CharonModules limine_modules_to_charon(struct limine_module_response *modules)
{
    CharonModules ret;
    ret.count = modules->module_count;
    for (size_t i = 0; i < ret.count; i++)
    {
        ret.modules[i] = (CharonModule){
            .name = modules->modules[i]->path,
            .address = (uintptr_t)modules->modules[i]->address,
            .size = modules->modules[i]->size};
    }
    return ret;
}

Charon limine_to_charon()
{
    Charon ret = {0};

    if (rsdp_request.response)
    {
        ret.rsdp = (uintptr_t)rsdp_request.response->address;
    }

    if (fb_request.response && fb_request.response->framebuffer_count != 0)
    {
        ret.framebuffer = limine_fb_to_charon(fb_request.response->framebuffers[0]);
    }

    if (memmap_request.response)
    {
        ret.memory_map = limine_mmap_to_charon(memmap_request.response);
    }

    if (module_request.response)
    {
        ret.modules = limine_modules_to_charon(module_request.response);
    }

    return ret;
}
