/* @license:bsd2 */
#pragma once

#include "kernel/sched.hpp"
#include <lib/list.hpp>
#include <lib/spinlock.hpp>

namespace Gaia {

struct Waitq {
  Spinlock lock;
  List<Thread, &Thread::wait_link> waiters;

  /**
   * @brief Wait for the wait queue to be triggered
   * @param timeout The timeout in microseconds, any value lower or equal to 0
   * means no timeout
   */
  Result<Void, Error> await(uint64_t timeout);

  /**
   * @brief Wake one or more threads from the wait queue
   * @param n The number of threads to wake
   */
  Result<Void, Error> wake(int n);
};

struct Waitable {
  Waitq wq;
  auto trigger_event(int n = -1) { return wq.wake(n); }
  auto await_event(uint64_t timeout) { return wq.await(timeout); }
};

} // namespace Gaia