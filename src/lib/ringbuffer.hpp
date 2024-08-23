/* @license:b sd2 */
#pragma once
#include <lib/base.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>
#include <stddef.h>

namespace Gaia {

template <typename T, size_t N> class Ringbuffer {
public:
  Result<Void, Error> push(T data) {

    if ((write_index + 1) % N == read_index) {
      return Err(Error::FULL);
    }

    buffer[write_index] = data;

    write_index = (write_index + 1) % N;

    return Ok({});
  }

  Result<T, Error> pop() {
    if (read_index == write_index)
      return Err(Error::EMPTY);

    auto ret = buffer[read_index];

    read_index = (read_index + 1) % N;
    return Ok(ret);
  }

  Result<T, Error> peek() const {
    if (read_index == write_index)
      return Err(Error::EMPTY);
    return Ok(buffer[read_index]);
  }

  size_t size() const { return (write_index - read_index); }

private:
  size_t write_index = 0, read_index = 0;
  T buffer[N];
};

} // namespace Gaia