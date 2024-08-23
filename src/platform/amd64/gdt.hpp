/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

namespace Gaia::Amd64 {
void gdt_init();
void gdt_init_tss();
} // namespace Gaia::Amd64
