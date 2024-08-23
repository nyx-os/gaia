/* @license:bsd2 */
#pragma once
#include <frg/spinlock.hpp>

namespace Gaia {

class Spinlock {
public:
  void lock() { _lock.lock(); }
  void unlock() { _lock.unlock(); }

private:
  frg::simple_spinlock _lock;
};

} // namespace Gaia