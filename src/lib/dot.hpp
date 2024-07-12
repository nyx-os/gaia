/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file dot.hpp
 * @brief Utilities to generate the DOT file format
 *
 */
#pragma once

#include <frg/formatting.hpp>
#include <frg/logging.hpp>
#include <frg/string.hpp>

namespace Gaia {

template <typename Allocator> class DotGraph {
public:
  explicit DotGraph(bool directed) : directed(directed) {
    str += directed ? "digraph {" : "graph {";
  }

  /**
   * @brief Adds a child node to a parent
   *
   * @param parent The parent to add a child to
   * @param child The child's node id
   * @param child_label The child's label
   */
  void add_child_to_node(frg::string_view parent, frg::string_view child,
                         frg::string_view child_label) {

    add_node(child, child_label);
    frg::output_to(str) << frg::fmt("{}{}{};", parent, directed ? "->" : "--",
                                    child);
  }

  void add_node(frg::string_view name, frg::string_view label) {
    frg::output_to(str) << frg::fmt("{}[label=\"{}\"];", name, label);
  }

  /**
   * @brief Generates the DOT graph string, NOTE: this can only be called once
   *
   * @return The generated string
   */
  frg::string<Allocator> generate() {
    str.push_back('}');
    return str;
  }

  /**
   * @brief Gets the DOT graph string
   *
   * @return The string
   */
  frg::string<Allocator> get_str() { return str; }

private:
  bool directed = false;
  frg::string<Allocator> str = "";
};

} // namespace Gaia
