/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file error.hpp
 * @brief Defines a generic Error type and associated utilities
 *
 * This files describes a generic Error type for use in the Result class.
 */

#pragma once

#include <frg/string.hpp>
namespace Gaia {

/// A generic error type
enum class Error {

  UNKNOWN,
  NOT_IMPLEMENTED,
  NO_SUCH_FILE_OR_DIRECTORY,
  IS_A_DIRECTORY,
  OUT_OF_MEMORY,
  INVALID_PARAMETERS,
  PERMISSION_DENIED,
  NOT_FOUND,
  TYPE_MISMATCH,
  NOT_A_DIRECTORY,
  NOT_A_TTY,
  INVALID_FILE,
  FULL,
  EMPTY,
};

/**
 * @brief Converts an Error to a string
 *
 * @param error The error to convert
 * @return The error converted to a string
 */
inline frg::string_view error_to_string(Error error) {
  switch (error) {
  case Error::UNKNOWN:
    return "Unknown error";
  case Error::NOT_IMPLEMENTED:
    return "Not implemented";
  case Error::NO_SUCH_FILE_OR_DIRECTORY:
    return "No such file or directory";
  case Error::IS_A_DIRECTORY:
    return "Is a directory";
  case Error::OUT_OF_MEMORY:
    return "Out of memory";
  case Error::INVALID_PARAMETERS:
    return "Invalid parameters";
  case Error::PERMISSION_DENIED:
    return "Permission denied";
  case Error::NOT_FOUND:
    return "Not found";
  case Error::TYPE_MISMATCH:
    return "Type mismatch";
  case Error::NOT_A_DIRECTORY:
    return "Not a directory";
  case Error::NOT_A_TTY:
    return "Not a TTY";
  case Error::INVALID_FILE:
    return "Invalid file";
  case Error::FULL:
    return "Full";
  case Error::EMPTY:
    return "Empty";
  }

  return "";
}

} // namespace Gaia
