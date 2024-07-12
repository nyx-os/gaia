/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <cstdint>
#include <lib/base.hpp>
#include <vm/heap.hpp>

namespace Gaia {

constexpr auto MAX_LISTENERS = 32;

struct EventListener {
  void *thread;
  size_t which;
};

struct Event {
  Vm::Vector<EventListener> listeners;
  size_t pending;

  void trigger(bool drop = false);
  void add_listener(void *thread, size_t which);
  void remove_listener(void *thread);

  Event() : pending(0) {}
};

uint64_t await(Vm::Vector<Event *> events);

} // namespace Gaia