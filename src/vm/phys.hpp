/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <lib/charon.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia::Vm {
void phys_init(Charon charon);

Result<void *, Error> phys_alloc(bool zero = false);

void phys_free(void *page);

size_t phys_usable_pages();
uintptr_t phys_highest_usable_page();
size_t phys_total_pages();

} // namespace Gaia::Vm
