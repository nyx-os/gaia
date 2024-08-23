/* SPDX-License-Identifier: BSD-2-Clause */
#include <amd64/limine.h>
#include <kernel/main.hpp>

using namespace Gaia;

volatile static struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST, .revision = 0};

volatile static struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

volatile static struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

volatile static struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST, .revision = 0};
static Result<CharonMmapEntryType, Error>
limine_mmap_type_to_charon(uint64_t type) {
  switch (type) {
  case LIMINE_MEMMAP_USABLE:
    return Ok(FREE);
  case LIMINE_MEMMAP_RESERVED:
    return Ok(RESERVED);
  case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
    return Ok(RECLAIMABLE);
  case LIMINE_MEMMAP_ACPI_NVS:
    return Ok(RESERVED);
  case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
    return Ok(RECLAIMABLE);
  case LIMINE_MEMMAP_KERNEL_AND_MODULES:
    return Ok(MODULE);
  case LIMINE_MEMMAP_FRAMEBUFFER:
    return Ok(FRAMEBUFFER);
  }

  return Err(Error::INVALID_PARAMETERS);
}

static void limine_mmap_to_charon(struct limine_memmap_response *mmap,
                                  CharonMemoryMap *ret) {
  ret->count = mmap->entry_count;

  for (size_t i = 0; i < mmap->entry_count; i++) {
    ret->entries[i] = (CharonMmapEntry){
        .type = limine_mmap_type_to_charon(mmap->entries[i]->type).unwrap(),
        .base = mmap->entries[i]->base,
        .size = mmap->entries[i]->length};
  }
}

static void limine_modules_to_charon(struct limine_module_response *modules,
                                     CharonModules *ret) {
  ret->count = modules->module_count;
  for (size_t i = 0; i < ret->count; i++) {
    ret->modules[i].address = (uintptr_t)modules->modules[i]->address;
    ret->modules[i].size = modules->modules[i]->size;
    ret->modules[i].name = modules->modules[i]->path;
  }
}

Charon make_charon() {
  Charon charon = {};

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

    charon.framebuffer = (CharonFramebuffer){.present = true,
                                             .address = (uintptr_t)fb->address,
                                             .width = (uint32_t)fb->width,
                                             .height = (uint32_t)fb->height,
                                             .pitch = (uint32_t)fb->pitch,
                                             .bpp = fb->bpp};
  }

  return charon;
}

extern "C" void _start() {
  log("Hello from aarch64");

  auto charon = make_charon();

  auto ret = main(charon);

  if (ret.is_err()) {
    panic("Main() exited with error: {}", error_to_string(ret.error().value()));
  }

  for (;;) {
    asm("wfi");
  }
}
