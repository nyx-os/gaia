/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/acpi/acpi.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>
#include <vm/heap.hpp>

namespace Gaia::Dev {
class AcpiDevice : public Service {
public:
  AcpiDevice(lai_variable_t id, lai_nsnode_t *obj);

  void start(Service *provider) override;

  lai_nsnode_t *aml_node;
  lai_variable_t id;

  bool match_properties(Properties &props) override;
  const char *class_name() override { return "AcpiDevice"; }
  const char *name() override { return _name.data(); };

  Result<lai_nsnode_t *, Error> resolve_path(const char *path);
  Result<uint64_t, Error> eval_path_int(const char *path);

private:
  frg::string<Gaia::Vm::HeapAllocator> _name = "";
  const char *devid;
  Vm::UniquePtr<Service> driver;
};
} // namespace Gaia::Dev
