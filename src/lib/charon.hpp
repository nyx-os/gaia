/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file charon.hpp
 * @brief Charon is gaia's generic (i.e bootloader-independent) boot protocol.
 *
 * A Charon object is built by the architecture-dependent code then it is
 * passed to Gaia::Main() in order to ensure unified kernel entry.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Gaia {
constexpr auto CHARON_MODULE_MAX = 16; /**< The maximum number of modules */
constexpr auto CHARON_MMAP_SIZE_MAX =
    48; /**< The maximum number of memory map entries */

enum CharonMmapEntryType : uint8_t {
  FREE,        ///< Denotes a free memory map entry
  RESERVED,    ///< Denotes a reserved memory map entry
  MODULE,      ///< Denotes a memory map entry that stores modules or the kernel
  RECLAIMABLE, ///< Denotes a reclaimable memory map entry
  FRAMEBUFFER  ///< Denotes a memory map entry that contains the framebuffer
};

/// Describes a memory map entry
struct CharonMmapEntry {
  CharonMmapEntryType type; ///< Type
  uintptr_t base;           ///< Base address
  size_t size;              ///< Size
};

/// Describes the physical memory map
struct CharonMemoryMap {
  size_t count = 0; ///< Number of entries

  CharonMmapEntry *begin() { return entries; }
  CharonMmapEntry *end() { return entries + count; }

  CharonMmapEntry entries[CHARON_MMAP_SIZE_MAX]; /// The actual entries
};

/// Describes the framebuffer
struct CharonFramebuffer {
  bool present;      ///< Whether the framebuffer exists or not
  uintptr_t address; ///< Address of the framebuffer is located
  uint32_t width, height, pitch, bpp;
  uint8_t red_mask_size;
  uint8_t red_mask_shift;
  uint8_t green_mask_size;
  uint8_t green_mask_shift;
  uint8_t blue_mask_size;
  uint8_t blue_mask_shift;
};

/// Describes a single bootloader module
struct CharonModule {
  size_t size = 0;       ///< Size of the module
  uintptr_t address = 0; ///< Base address of the module (virtual)
  const char *name;      ///< Name of the module
};

/// Array of modules
struct CharonModules {
  uint16_t count = 0;                      ///< The number of modules
  CharonModule modules[CHARON_MODULE_MAX]; ///< The actual modules
};

struct Charon {
  uintptr_t rsdp; ///< Root system description pointer, used by ACPI routines.
  CharonFramebuffer framebuffer; ///< The framebuffer
  CharonModules modules;         ///< The bootloader modules
  CharonMemoryMap memory_map;    ///< The memory map
};

} // namespace Gaia
