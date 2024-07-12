/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

namespace Gaia::x86_64 {

void simd_init();
void simd_save_state(void *state);
void simd_restore_state(void *state);
void simd_init_context(void *state);
} // namespace Gaia::x86_64