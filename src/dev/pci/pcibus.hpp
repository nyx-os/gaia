/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/acpi/device.hpp>
#include <dev/devkit/registry.hpp>
#include <dev/devkit/service.hpp>
#include <vm/heap.hpp>

namespace Gaia::Dev {

class PciBus : public Service {
public:
  void start(Service *provider) override;

  static Vm::UniquePtr<Service> init();

  const char *class_name() override { return "PciBus"; }

  const char *name() override { return name_str.data(); }

private:
  AcpiDevice *acpi_device;
  frg::string<Vm::HeapAllocator> name_str = "";
  uint64_t bus = 0;
};

void pcibus_driver_register();

} // namespace Gaia::Dev
