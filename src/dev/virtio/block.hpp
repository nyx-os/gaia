/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/virtio/virtio.hpp>
#include <vm/heap.hpp>

namespace Gaia::Dev {

class VirtioBlock : public Service {
public:
  static Vm::UniquePtr<Service> create();
  void start(Service *provider) override;

  const char *class_name() override { return "VirtioBlock"; }
  const char *name() override { return name_str.data(); }

private:
  frg::string<Gaia::Vm::HeapAllocator> name_str = "";
  VirtioDevice *device;
  VirtQueue queue;
};

void virtioblock_driver_register();

} // namespace Gaia::Dev
