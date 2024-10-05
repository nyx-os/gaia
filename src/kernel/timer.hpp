/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/pairing_heap.hpp>
#include <kernel/wait.hpp>

namespace Gaia {

struct Timer : public Waitable {
  frg::pairing_heap_hook<Timer> hook;

  Timer(void (*callback)(), uint64_t ms) : callback(callback), timeout(ms) {}

  enum {
    DISABLED,
    PENDING,
    CANCELLED,
    COMPLETED,
  } state;

  void (*callback)();
  uint64_t deadline = 0, timeout = 0;
};

Result<Void, Error> timer_enqueue(Timer *timer);

void timer_cancel(Timer *timer);

void timer_interrupt();

} // namespace Gaia
