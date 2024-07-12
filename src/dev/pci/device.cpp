#include <dev/devkit/registry.hpp>
#include <dev/pci/device.hpp>
#include <frg/span.hpp>
#include <lai/helpers/pci.h>
#include <lib/log.hpp>

namespace Gaia::Dev {

static int num = 0;

PciDevice::PciDevice(uint8_t bus, uint8_t slot, uint8_t fun)
    : bus(bus), slot(slot), fun(fun), driver(Vm::get_allocator()) {

  info.slot = slot;
  info.bus = bus;
  info.fun = fun;

  frg::output_to(_name) << frg::fmt("PciDevice{}", num++);
}

bool PciDevice::match_properties(Properties &props) {
  if (props.get("vendor") && !props.get("deviceId")) {
    return vendor == props["vendor"].integer;
  }

  return vendor == props["vendor"].integer &&
         device_id == props["deviceId"].integer;
}

void PciDevice::start(Service *provider) {
  attach(provider);

  auto props = Properties{frg::hash<frg::string_view>{}};

  vendor = read<uint16_t>(PciOffset::VENDOR_ID);
  device_id = read<uint16_t>(PciOffset::DEVICE_ID);

  props["vendor"].integer = vendor;
  props["deviceId"].integer = device_id;
  props["provider"].string = class_name();

  auto drv_res = get_catalog().find_driver(this);

  if (!drv_res.is_ok()) {
    // log("No driver found for pci {:x}:{:x}", props["vendor"].integer,
    //   props["deviceId"].integer);
  } else {
    driver = drv_res.unwrap();
    info.vendor = vendor;
    info.device_id = device_id;
    info._class = read<uint8_t>(PciOffset::CLASS);
    info.subclass = read<uint8_t>(PciOffset::SUBCLASS);
    info.pin = read<uint8_t>(PciOffset::PIN);

    if (info.pin) {
      acpi_resource_t res;
      auto r =
          lai_pci_route_pin(&res, 0, info.bus, info.slot, info.fun, info.pin);
      if (r != LAI_ERROR_NONE) {
        panic("lai failed to route pin, error code: {}", r);
      } else {
        info.gsi = res.base;
        info.lopol = res.irq_flags & ACPI_SMALL_IRQ_ACTIVE_LOW;
        info.edge = res.irq_flags & ACPI_SMALL_IRQ_EDGE_TRIGGERED;
      }
    }

    driver->start(this);
  }
}

frg::span<uint8_t> PciDevice::get_bar(uint8_t num) {

  size_t reg_index = 0x10 + num * 4;
  auto offset = (PciOffset)reg_index;
  auto bar_low = read<uint32_t>(offset);
  uintptr_t bar_high = 0;
  size_t bar_size_low = 0, bar_size_high = ~0;

  uintptr_t base = 0;
  size_t size = 0;

  int is_mmio = !(bar_low & 1);
  int is_64bit = is_mmio && ((bar_low >> 1) & 0b11) == 0b10;

  if (is_64bit)
    bar_high = read<uint32_t>((PciOffset)(reg_index + 4));

  base = ((bar_high << 32) | bar_low) & ~(is_mmio ? (0b1111) : (0b11));

  write<uint32_t>(offset, ~0);
  bar_size_low = read<uint32_t>(offset);
  write<uint32_t>(offset, bar_low);

  if (is_64bit) {
    write<uint32_t>((PciOffset)(reg_index + 4), ~0);
    bar_size_high = read<uint32_t>((PciOffset)(reg_index + 4));
    write<uint32_t>((PciOffset)(reg_index + 4), bar_high);
  }

  size =
      ((bar_size_high << 32) | bar_size_low) & ~(is_mmio ? (0b1111) : (0b11));
  size = ~size + 1;

  return {(uint8_t *)base, size};
}

} // namespace Gaia::Dev