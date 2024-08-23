#include "frg/pairing_heap.hpp"
#include "hal/int.hpp"
#include <kernel/timer.hpp>

namespace Gaia {

struct TimerComp {
  bool operator()(const Timer *a, const Timer *b) const {
    return a->timeout > b->timeout;
  }
};

static frg::pairing_heap<
    Timer,
    frg::locate_member<Timer, frg::pairing_heap_hook<Timer>, &Timer::hook>,
    TimerComp>
    timer_heap;

Result<Void, Error> timer_enqueue(Timer *timer) {
  timer_heap.push(timer);
  timer->state = Timer::PENDING;
  Hal::set_timer(timer->timeout);

  return Ok({});
}

void timer_interrupt() {
  auto top = timer_heap.top();
  uint64_t passed = 0;

  if (top->state == Timer::CANCELLED) {
    timer_heap.pop();
    passed = top->timeout;
    top = timer_heap.top();
  } else {
    passed = top->timeout;
  }

  while (true) {
    if (timer_heap.empty()) {
      Hal::set_timer(0);
      break;
    }

    if (timer_heap.top()->timeout - passed > 0) {
      timer_heap.top()->timeout -= passed;
      break;
    }

    auto timer = timer_heap.top();
    timer_heap.pop();

    ASSERT(timer->state == Timer::PENDING);

    log("Timer of {} finished", timer->timeout);
    timer->state = Timer::COMPLETED;
    timer->callback();
  }
}

void timer_cancel(Timer *timer) { timer->state = Timer::CANCELLED; }

} // namespace Gaia
