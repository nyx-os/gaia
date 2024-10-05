#include <frg/pairing_heap.hpp>
#include <kernel/cpu.hpp>
#include <kernel/timer.hpp>

namespace Gaia {

struct TimerComp {
  bool operator()(const Timer *a, const Timer *b) const {
    return a->deadline > b->deadline;
  }
};

static frg::pairing_heap<
    Timer,
    frg::locate_member<Timer, frg::pairing_heap_hook<Timer>, &Timer::hook>,
    TimerComp>
    timer_heap;

Result<Void, Error> timer_enqueue(Timer *timer) {
  timer->state = Timer::PENDING;
  timer->deadline = cpu_self()->ms + timer->timeout;
  timer_heap.push(timer);
  return Ok({});
}

void timer_interrupt() {
  cpu_self()->timer_lock.lock();
  cpu_self()->ms++;

  auto top = timer_heap.top();

  if (!top) {
    cpu_self()->timer_lock.unlock();
    return;
  }

  if (top->state == Timer::CANCELLED) {
    timer_heap.pop();
  }

  while (timer_heap.top()->deadline <= cpu_self()->ms) {
    auto timer = timer_heap.top();
    timer_heap.pop();

    ASSERT(timer->state == Timer::PENDING);

    timer->state = Timer::COMPLETED;

    if (timer->callback)
      timer->callback();
    timer->trigger_event().unwrap();

    if (!timer_heap.top())
      break;
  }

  cpu_self()->timer_lock.unlock();
}

void timer_cancel(Timer *timer) { timer->state = Timer::CANCELLED; }

} // namespace Gaia
