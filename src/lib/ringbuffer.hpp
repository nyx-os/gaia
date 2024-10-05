/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <lib/base.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>
#include <stddef.h>

namespace Gaia {

template <typename T, size_t N> class Ringbuffer {
public:
  Result<Void, Error> push(T data) {
    if (is_full()) {
      tail_index = (tail_index + 1) % N;
    }

    buffer[head_index] = data;

    head_index = (head_index + 1) % N;

    return Ok({});
  }

  Result<T, Error> pop() {
    if (is_empty())
      return Err(Error::EMPTY);

    auto ret = buffer[tail_index];

    tail_index = (tail_index + 1) % N;
    return Ok(ret);
  }

  // This is useful for e.g the TTY where you need to erase the last character
  Result<Void, Error> erase_last() {
    if (is_empty()) {
      return Err(Error::EMPTY);
    }

    head_index = (head_index + N - 1) % N;
    return Ok({});
  }

  Result<T, Error> peek() const {
    if (is_empty())
      return Err(Error::EMPTY);
    return Ok(buffer[tail_index]);
  }

  Result<T, Error> peek(size_t index) const {
    if (is_empty())
      return Err(Error::EMPTY);

    size_t buffer_size = size();
    if (index >= buffer_size) {
      return Err(Error::INVALID_PARAMETERS);
    }

    size_t actual_index = (tail_index + index) % N;
    return Ok(buffer[actual_index]);
  }

  size_t size() const { return (head_index - tail_index) % N; }

  bool is_full() const { return (head_index - tail_index) % N == N; }

  bool is_empty() const { return (head_index == tail_index); }

private:
  size_t tail_index = 0, head_index = 0;
  T buffer[N];
};

} // namespace Gaia