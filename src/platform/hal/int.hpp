/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <kernel/ipl.hpp>
#include <lib/error.hpp>
#include <lib/result.hpp>

namespace Gaia::Hal {
struct InterruptFrame;
struct InterruptEntry;

using InterruptHandler = void(InterruptFrame *, void *arg);
Result<uint8_t, Error> allocate_interrupt(Ipl ipl, InterruptHandler *handler,
                                          void *arg, InterruptEntry *entry);

Result<uint8_t, Error>
register_interrupt_handler(int vector, Hal::InterruptHandler *handler,
                           void *arg, Hal::InterruptEntry *entry);

uint64_t get_timer_count();

// Returns seconds since epoch
uint64_t get_time_of_day();

void do_context_switch();

Ipl get_ipl();
void set_ipl(Ipl ipl);
} // namespace Gaia::Hal
