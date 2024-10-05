/* SPDX-License-Identifier: BSD-2-Clause */
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