#include "hal/hal.hpp"
#include <kernel/event.hpp>
#include <kernel/sched.hpp>

namespace Gaia {

void Event::trigger(bool drop) {

  if (listeners.size() == 0 && !drop) {
    pending++;
    return;
  }
  for (auto listener : listeners) {
    auto thread = reinterpret_cast<Thread *>(listener.thread);
    thread->which_event = listener.which;

    sched_wake_thread(thread);
  }
}

void Event::add_listener(void *thread, size_t which) {
  listeners.push(EventListener{thread, which});
}

void Event::remove_listener(void *thread) {
  for (size_t i = 0; i < listeners.size(); i++) {
    if (listeners[i].thread == thread) {
      listeners[i] = listeners.pop();
    }
  }
}

uint64_t await(Vm::Vector<Event *> events) {
  auto t = sched_curr();

  Hal::disable_interrupts();

  for (size_t i = 0; i < events.size(); i++) {
    if (events[i]->pending > 0) {
      events[i]->pending--;
      Hal::enable_interrupts();
      return i;
    }
  }

  for (size_t i = 0; i < events.size(); i++) {
    events[i]->add_listener(t, i);
  }

  sched_suspend_thread(t);

  for (auto event : events) {
    event->listeners.clear();
  }

  Hal::enable_interrupts();

  return t->which_event;
}

} // namespace Gaia