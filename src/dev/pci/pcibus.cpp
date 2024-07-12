#include <dev/acpi/acpi.hpp>
#include <dev/pci/device.hpp>
#include <dev/pci/pcibus.hpp>
#include <lai/core.h>

namespace Gaia::Dev {

static int bus_num = 0;

Vm::UniquePtr<Service> PciBus::init() {
  return {Vm::get_allocator(), new PciBus};
}

void PciBus::start(Service *provider) {
  attach(provider);

  frg::output_to(name_str) << frg::fmt("{}{}", class_name(), bus_num++);

  acpi_device = (AcpiDevice *)provider;

  bus = acpi_device->eval_path_int("_BBN").unwrap_or(0);

  for (int slot = 0; slot < 32; slot++) {
    uint32_t vendor = 0;
    size_t func_n = 1;

    if ((vendor = laihost_pci_readw(0, bus, slot, 0,
                                    (uint16_t)PciOffset::VENDOR_ID)) == 0xffff)
      continue;

    if (laihost_pci_readb(0, bus, slot, 0, (uint16_t)PciOffset::HEADER_TYPE) &
        (1 << 7))
      func_n = 8;

    for (size_t func = 0; func < func_n; func++) {
      if (laihost_pci_readw(0, bus, slot, func,
                            (uint16_t)PciOffset::VENDOR_ID) == 0xffff) {
        continue;
      }

      auto dev = new PciDevice(bus, slot, func);

      dev->start(this);
    }
  }

  log("pci enumeration complete");
}

void pcibus_driver_register() {
  auto props = new Properties{frg::hash<frg::string_view>{}};

  // PCIe bus HID
  props->insert("hid", {.string = "PNP0A08"});

  // PCI bus HID
  props->insert("altHid", {.string = "PNP0A03"});

  props->insert("provider", {.string = "AcpiDevice"});

  get_catalog().register_driver(Driver{&PciBus::init}, *props);
}

} // namespace Gaia::Dev