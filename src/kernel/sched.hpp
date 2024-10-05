// @license:bsd2
#pragma once
#include "frg/spinlock.hpp"
#include "kernel/cpu.hpp"
#include <fs/vfs.hpp>
#include <hal/int.hpp>
#include <hal/mmu.hpp>
#include <lib/list.hpp>
#include <posix/fd.hpp>
#include <sys/types.h>
#include <vm/vm.hpp>

// FIXME: Find an architecture-independent way of doing this
#include <amd64/idt.hpp>

namespace Gaia {

constexpr auto TIME_SLICE = 5;

struct Task;

struct Waitq;

enum class WaitResult {
  WAITING,
  SUCCESS,
  FAILED,
};

struct Thread {
  Hal::CpuContext ctx;
  Cpu *cpu;
  enum {
    RUNNING,
    SUSPENDED,
    EXITED,
  } state;

  Vm::String name;

  Task *task;

  Vm::Vector<Task> children;

  ListNode<Thread> link; // Scheduler queue

  bool in_fault = false;

  frg::simple_spinlock lock;

  ListNode<Thread> wait_link;
  Waitq *waitq = nullptr;
  WaitResult wait_res;

  ~Thread();
};

pid_t sched_allocate_pid();

Result<Thread *, Error> sched_new_thread(frg::string_view name, Task *task,
                                         Hal::CpuContext ctx, bool insert);

Result<Task *, Error> sched_new_task(pid_t pid, Task *parent, bool user);

void sched_tick(Hal::InterruptFrame *frame);

void sched_yield();

Result<Void, Error> sched_init();

Thread *sched_curr();

void sched_enqueue_thread(Thread *thread);
void sched_dequeue_thread(Thread *thread);

[[noreturn]] void sched_dequeue_and_die();

void sched_suspend_thread(Thread *thread);
void sched_wake_thread(Thread *thread);

void sched_send_to_death(Thread *thread);

Result<Thread *, Error> sched_new_worker_thread(frg::string_view name,
                                                uintptr_t entry_point,
                                                bool insert = true);

Task *sched_kernel_task();

void sched_register_cpu(Cpu *cpu);

} // namespace Gaia
