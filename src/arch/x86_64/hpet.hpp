/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <cstdint>
#include <dev/acpi/acpi.hpp>

namespace Gaia::x86_64 {
void hpet_init(Dev::AcpiPc *acpi);
void hpet_sleep(uint64_t ms);
bool hpet_present();

struct [[gnu::packed]] Hpet : Dev::AcpiTable {
  uint8_t hardware_rev_id;
  uint8_t info;
  uint16_t pci_vendor_id;
  uint8_t address_space_id;
  uint8_t register_bit_width;
  uint8_t register_bit_offset;
  uint8_t reserved1;
  uint64_t address;
  uint8_t hpet_number;
  uint16_t minimum_tick;
  uint8_t page_protection;
};
} // namespace Gaia::x86_64