/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/acpi/acpi.hpp>
#include <lib/base.hpp>

namespace Gaia::Dev {
void lai_glue_init(uintptr_t rsdp, AcpiTable *_sdt, bool xsdt);
} // namespace Gaia::Dev