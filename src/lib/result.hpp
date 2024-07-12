/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file result.hpp
 * @brief Defines the Ok, Err and Result classes
 */
#pragma once
#include <frg/variant.hpp>
#include <lib/log.hpp>
#ifdef __KERNEL__
#include <posix/errno.hpp>
#endif
#include <utility>

namespace Gaia {

/// An empty struct to allow Result's T to be void
struct Void {};

/// Represents a success value
template <typename T = Void> class Ok {
public:
  constexpr explicit Ok(T value) : value(std::move(value)) {}

  T value;
};

/// Represents an error value
template <typename T> class Err {
public:
  constexpr explicit Err(T value) : value(value) {}
  T value;
};

/**
 * @brief Represents either a value or an error.
 *
 * @tparam T The type of the value.
 * @tparam E The type of the error.
 */
template <typename T, typename E> class Result {
public:
  constexpr Result(Ok<T> value) : variant(std::move(value)) {}
  constexpr Result(Err<E> value) : variant(std::move(value)) {}

  constexpr bool is_ok() { return variant.template is<Ok<T>>(); }

  constexpr bool is_err() { return variant.template is<Err<E>>(); }

  /**
   * @brief Gets the Ok value if present and panics if called on an Err value
   *
   * @param fmt Panic message when called on an Err value (optional)
   * @return The Ok value contained
   */
  constexpr T
  unwrap(const FormatWithLocation &fmt =
             "unwrap failed: cannot unwrap an error type, err: {}") {
    if (is_ok())
      return value().value();
    panic(fmt, error_to_string(error().value()));
  }

  /**
   * @brief Gets the Ok value if present or returns a default value
   *
   * @param _value The value returned on error
   * @return The Ok value or _value
   */
  constexpr T unwrap_or(T _value) {
    if (is_ok())
      return value().value();
    return _value;
  }

  /**
   * @brief Gets the Err value
   *
   * @return the Err value if present or frg::null_opt otherwise
   */
  constexpr frg::optional<E> error() {
    if (is_err())
      return variant.template get<Err<E>>().value;
    return frg::null_opt;
  }

  /**
   * @brief Gets the Ok value
   *
   * @return the Ok value if present or frg::null_opt otherwise
   */
  constexpr frg::optional<T> value() {
    if (is_ok())
      return std::move(variant.template get<Ok<T>>().value);
    return frg::null_opt;
  }

private:
  frg::variant<Ok<T>, Err<E>> variant;
};

#define TRY(X)                                                                 \
  ({                                                                           \
    auto __ret = (X);                                                          \
    if (__ret.is_err())                                                        \
      return Err(__ret.error().value());                                       \
    __ret.value().value();                                                     \
  })

} // namespace Gaia
