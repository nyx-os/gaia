/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <lib/charon.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia {
Result<Void, Error> main(Charon charon);
Charon &charon();
} // namespace Gaia
