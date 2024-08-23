/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <dev/acpi/acpi.hpp>

namespace Gaia::Amd64 {

void timer_init(Dev::AcpiPc *acpi);
void timer_sleep(uint64_t ms);

} // namespace Gaia::Amd64
