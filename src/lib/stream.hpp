/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/span.hpp>
#include <lib/base.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>
#include <sys/types.h>

namespace Gaia {

struct Stream {
  enum class Whence {
    CURRENT,
    SET,
    END,
  };

  virtual Result<size_t, Error> write(void *buf, size_t size) = 0;
  virtual Result<size_t, Error> read(void *buf, size_t size) = 0;
  virtual Result<off_t, Error> seek(off_t offset, Whence whence) = 0;
};

} // namespace Gaia