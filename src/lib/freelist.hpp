/* SPDX-License-Identifier: BSD-2-Clause */
/**
 * @file freelist.hpp
 * @brief Freelist allocator implementation
 *
 */
#pragma once
#include <cstdint>
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia {

class Freelist {
public:
  /// A chunk of memory
  struct Region {
    size_t size;  ///< Size of the region
    Region *next; ///< Pointer to the next free region
  };

  /**
   * @brief Construct a new Freelist object
   *
   * @param quantum The number of bytes that are allocated in one allocation
   */
  explicit Freelist(size_t quantum) : quantum(quantum) {}
  Freelist() : quantum(1) {}

  /**
   * @brief Sets the quantum
   *
   * @param new_quantum The new quantum
   */
  void set_quantum(size_t new_quantum) { quantum = new_quantum; }

  /**
   * @brief Adds a region to the allocator
   *
   * @param region The region to add
   */
  void add_region(Region *region) {
    region->next = head;
    head = region;
  }

  /**
   * @brief Allocates `quantum` bytes
   *
   * @return The allocation on success, the error on failure.
   */
  Result<uintptr_t, Error> alloc() {
    if (!head) {
      return Err(Error::OUT_OF_MEMORY);
    }

    auto region = head;
    region->size -= quantum;

    if (!head->size) {
      head = head->next;
    }

    return Ok((uintptr_t)region + region->size);
  }

  /**
   * @brief Frees `quantum` bytes from address
   *
   * @param mem The address to free
   */
  void free(uintptr_t mem) {
    auto region = reinterpret_cast<Region *>(mem);
    region->size = quantum;
    add_region(region);
  }

private:
  Region *head = nullptr;
  size_t quantum = 0;
};

} // namespace Gaia
