/* SPDX-License-Identifier: BSD-2-Clause */
#include <machdep/machdep.h>
#include <kern/charon.h>
#include <x86_64/limine.h>

volatile static struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

volatile static struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

volatile static struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

volatile static struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

static enum charon_mmap_entry_type limine_mmap_type_to_charon(uint64_t type)
{
    switch (type) {
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
    }

    panic("Invalid limine memory map entry type: %ld", type);
    return -1;
}

static void limine_mmap_to_charon(struct limine_memmap_response *mmap,
                                  charon_mmap_t *ret)
{
    ret->count = mmap->entry_count;

    assert(ret->count <= CHARON_MMAP_SIZE_MAX);
    for (size_t i = 0; i < mmap->entry_count; i++) {
        ret->entries[i] = (charon_mmap_entry_t){
            .type = limine_mmap_type_to_charon(mmap->entries[i]->type),
            .base = mmap->entries[i]->base,
            .size = mmap->entries[i]->length
        };
    }
}

static void limine_modules_to_charon(struct limine_module_response *modules,
                                     charon_modules_t *ret)
{
    ret->count = modules->module_count;
    for (size_t i = 0; i < ret->count; i++) {
        ret->modules[i].address = (uintptr_t)modules->modules[i]->address;
        ret->modules[i].size = modules->modules[i]->size;
        ret->modules[i].name = modules->modules[i]->path;
    }
}

void gaia_main(charon_t charon);

void _start(void)
{
    charon_t charon = { 0 };

    if (memmap_request.response != NULL) {
        limine_mmap_to_charon(memmap_request.response, &charon.memory_map);
    }

    if (module_request.response != NULL) {
        limine_modules_to_charon(module_request.response, &charon.modules);
    }

    if (rsdp_request.response->address != NULL) {
        charon.rsdp = (uintptr_t)rsdp_request.response->address;
    }

    if (fb_request.response != NULL) {
        struct limine_framebuffer *fb = fb_request.response->framebuffers[0];

        charon.framebuffer =
                (charon_framebuffer_t){ .address = (uintptr_t)fb->address,
                                        .bpp = fb->bpp,
                                        .height = fb->height,
                                        .pitch = fb->pitch,
                                        .width = fb->width,
                                        .present = true };
    }

    gaia_main(charon);
    cpu_halt();
}
