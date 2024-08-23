/* @license:bsd2 */
#pragma once
#include <kernel/ipl.hpp>

namespace Gaia {
struct Cpu;
struct Thread;
} // namespace Gaia

namespace Gaia::Hal {

Gaia::Thread *get_current_thread();
void set_current_thread(Gaia::Thread *thread);

} // namespace Gaia::Hal