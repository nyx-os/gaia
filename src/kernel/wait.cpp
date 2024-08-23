#include "kernel/ipl.hpp"
#include "kernel/sched.hpp"
#include <kernel/wait.hpp>

using namespace Gaia;

Result<Void, Error> Waitq::await(uint64_t timeout) {
  ASSERT((int64_t)timeout <= 0 && "Not implemented!");

  auto ipl = iplx(Ipl::HIGH);

  auto thread = sched_curr();
  lock.lock();

  waiters.insert_tail(thread);
  thread->waitq = this;
  thread->wait_res = WaitResult::WAITING;

  lock.unlock();

  sched_suspend_thread(thread);
  iplx(ipl);

  if (thread->wait_res != WaitResult::SUCCESS) {
    return Err(Error::UNKNOWN);
  }

  return Ok({});
}

Result<Void, Error> Waitq::wake(int n) {
  if (n == -1) {
    n = waiters.length();
  }

  if (n > (int)waiters.length()) {
    return Err(Error::INVALID_PARAMETERS);
  }

  for (int i = 0; i < n; i++) {
    auto thread = TRY(waiters.remove_head());
    thread->wait_res = WaitResult::SUCCESS;
    sched_wake_thread(thread);
  }

  return Ok({});
}
