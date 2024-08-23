/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <amd64/idt.hpp>
#include <dev/acpi/device.hpp>
#include <dev/devkit/registry.hpp>
#include <dev/devkit/service.hpp>
#include <vm/heap.hpp>

namespace Gaia::Dev {

class Ps2Controller : public Service {
public:
  static void int_handler(Hal::InterruptFrame *frame, void *arg);

  void start(Service *provider) override;

  static Vm::UniquePtr<Service> init();

  const char *class_name() override { return "Ps2Controller"; }

  const char *name() override { return name_str.data(); }

private:
  AcpiDevice *acpi_device;
  frg::string<Vm::HeapAllocator> name_str = "";
  Hal::InterruptEntry int_entry;

  bool shift = false, capslock = false, ctrl = false;

  enum Scancode {
    MAX = 0x57,
    CTRL = 0x1d,
    CTRL_REL = 0x9d,
    SHIFT_RIGHT = 0x36,
    SHIFT_RIGHT_REL = 0xb6,
    SHIFT_LEFT = 0x2a,
    SHIFT_LEFT_REL = 0xaa,
    ALT_LEFT = 0x38,
    ALT_LEFT_REL = 0xb8,
    CAPSLOCK = 0x3a,
    NUMLOCK = 0x45,
  };

  static constexpr auto CAPS_LOCK_LED_ENABLE = (1 << 2);
};

void ps2_driver_register();

} // namespace Gaia::Dev
