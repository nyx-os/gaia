/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file path.hpp
 * @brief Path parser
 *
 */
#pragma once
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <lib/log.hpp>

namespace Gaia {

/**
 * @brief Describes a path
 *
 * @tparam Allocator The allocator to use
 */
template <typename Allocator> class Path {
public:
  using PathList = frg::vector<frg::string<Allocator>, Allocator>;

  /**
   * @brief Construct a new Path object
   *
   * @param path The path string
   */
  Path(frg::string_view path) : path(path) { vector.clear(); }

  /**
   * @brief Parses the path
   *
   * @return An array of strings representing the segments of the path
   */
  PathList parse() {
    if (vector.size() > 0)
      return vector;

    if (path.size() == 0)
      return vector;

    PathList ret{};

    frg::string<Allocator> new_path = path.data();

    // Root is considered a directory
    if (path[0] == '/') {
      ret.push_back("/");
    }

    size_t new_size = new_path.size();
    if (new_size > 1 && new_path[new_size - 1] == '/') {
      while (new_size > 1 && new_path[new_size - 1] == '/') {
        new_size--;
      }
      new_path.resize(new_size);
    }

    // Handle edge case where path is root
    if (new_path == "/") {
      vector.push("/");
      return vector;
    }

    char *sub = new_path.data();
    char *next = sub;
    size_t sublen = 0;
    bool last = false;

    // This code is kinda crappy and likely unsafe
    while (true) {
      sublen = 0;
      next = sub;

      while (*next != '/' && *next) {
        next++;
        sublen++;
      }

      if (!*next) {
        last = true;
      } else {
        *next = '\0';
      }

      if (sublen == 0 && !last) {
        sub += sublen + 1;
        continue;
      }

      ret.push_back(sub);

      if (last)
        break;

      sub += sublen + 1;
    }

    vector = ret;

    return ret;
  }

  /**
   * @return The number of segments the path has
   */
  size_t length() { return vector.size(); }

private:
  frg::string_view path;
  PathList vector{};
};

} // namespace Gaia