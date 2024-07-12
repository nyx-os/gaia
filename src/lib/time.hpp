/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <cstdint>

namespace Gaia {
struct Time {
  uint64_t hours;
  uint64_t minutes;
  uint64_t seconds;
  uint64_t milliseconds;
  uint64_t nanoseconds;
};
} // namespace Gaia
