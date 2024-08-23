/* @license:bsd2 */
#pragma once
#include <frg/pairing_heap.hpp>
#include <kernel/wait.hpp>

namespace Gaia {

struct Timer : public Waitable {
  frg::pairing_heap_hook<Timer> hook;

  void set(void (*callback)(), uint64_t us) {
    this->callback = callback;
    timeout = us;
  }

  enum {
    DISABLED,
    PENDING,
    CANCELLED,
    COMPLETED,
  } state;

  void (*callback)();
  uint64_t timeout;
};

Result<Void, Error> timer_enqueue(Timer *timer);

void timer_cancel(Timer *timer);

void timer_interrupt();

} // namespace Gaia