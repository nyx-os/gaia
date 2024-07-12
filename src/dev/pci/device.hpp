/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include "lai/host.h"
#include "vm/heap.hpp"
#include <dev/devkit/service.hpp>
#include <frg/span.hpp>

namespace Gaia::Dev {

enum class PciOffset : uint16_t {
  VENDOR_ID = 0x0,
  DEVICE_ID = 0x2,
  COMMAND = 0x4,
  STATUS = 0x6,
  SUBCLASS = 0xa,
  CLASS = 0xb,
  HEADER_TYPE = 0xe,
  BASE_ADDRESS = 0x10,
  CAPABILITIES_POINTER = 0x34, // Linked-list of capabilities
  PIN = 0x3d,
};

enum class PciStatusBit : uint8_t {
  CAPABILITIES_LIST =
      (1 << 4) // Whether the device implements the linked-list of caps
};

enum class Command {
  IO_SPACE = (1 << 0),
  MEM_SPACE = (1 << 1),
  BUS_MASTER = (1 << 2),
  INTERRUPT_DISABLE = (1 << 10),
};

class PciDevice : public Service {
public:
  PciDevice(uint8_t bus, uint8_t slot, uint8_t fun);

  template <typename T> T read(PciOffset offset) {
    if (sizeof(T) == sizeof(uint8_t)) {
      return laihost_pci_readb(0, bus, slot, fun, (uint16_t)offset);
    } else if (sizeof(T) == sizeof(uint16_t)) {
      return laihost_pci_readw(0, bus, slot, fun, (uint16_t)offset);
    } else if (sizeof(T) == sizeof(uint32_t)) {
      return laihost_pci_readd(0, bus, slot, fun, (uint16_t)offset);
    }
  }

  template <typename T> void write(PciOffset offset, T value) {
    if (sizeof(T) == sizeof(uint8_t)) {
      laihost_pci_writeb(0, bus, slot, fun, (uint16_t)offset, value);
    } else if (sizeof(T) == sizeof(uint16_t)) {
      laihost_pci_writew(0, bus, slot, fun, (uint16_t)offset, value);
    } else if (sizeof(T) == sizeof(uint32_t)) {
      laihost_pci_writed(0, bus, slot, fun, (uint16_t)offset, value);
    }
  }

  inline void enable_cmd_flag(Command flag) {
    write<uint16_t>(PciOffset::COMMAND,
                    read<uint16_t>(PciOffset::COMMAND) | (uint8_t)flag);
  }

  inline void disable_cmd_flag(Command flag) {
    write<uint16_t>(PciOffset::COMMAND,
                    read<uint16_t>(PciOffset::COMMAND) & ~(uint8_t)flag);
  }

  struct Info {
    uint8_t bus;
    uint8_t slot;
    uint8_t fun;
    uint8_t _class, subclass;
    uint16_t vendor, device_id;
    uint8_t pin;
    uint8_t gsi;
    bool lopol, edge;
  };

  Info get_info() { return info; };

  void start(Service *provider) override;

  bool match_properties(Properties &props) override;

  const char *class_name() override { return "PciDevice"; }
  const char *name() override { return _name.data(); };

  template <typename F> void iter_capabilities(F callback) {
    if (read<uint16_t>(PciOffset::STATUS) &
        (uint8_t)PciStatusBit::CAPABILITIES_LIST) {
      auto cap_off = read<uint8_t>(PciOffset::CAPABILITIES_POINTER);

      while (cap_off) {
        callback(cap_off);
        cap_off = read<uint8_t>(static_cast<PciOffset>(cap_off + 1));
      }
    }
  }

  inline void set_cmd(Command command, bool enabled) {
    if (enabled)
      enable_cmd_flag(command);
    else
      disable_cmd_flag(command);
  }

  inline void set_bus_mastering(bool enabled) {
    set_cmd(Command::BUS_MASTER, enabled);
  }

  inline void set_memory_space(bool enabled) {
    set_cmd(Command::MEM_SPACE, enabled);
  }

  inline void set_io_space(bool enabled) {
    set_cmd(Command::IO_SPACE, enabled);
  }

  frg::span<uint8_t> get_bar(uint8_t num);

private:
  uint8_t bus, slot, fun;
  uint16_t vendor, device_id;
  frg::string<Gaia::Vm::HeapAllocator> _name = "";
  Vm::UniquePtr<Service> driver;
  Info info;
};

} // namespace Gaia::Dev
