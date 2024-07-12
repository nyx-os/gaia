/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

namespace Gaia::x86_64 {
void gdt_init();
void gdt_init_tss();
} // namespace Gaia::x86_64
