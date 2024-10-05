#include "amd64/apic.hpp"
#include "elf.hpp"
#include "frg/spinlock.hpp"
#include "fs/vfs.hpp"
#include "hal/cpu.hpp"
#include "hal/hal.hpp"
#include "hal/int.hpp"
#include "kernel/cpu.hpp"
#include "lib/result.hpp"
#include "posix/fd.hpp"
#include "vm/phys.hpp"
#include "vm/vm_kernel.hpp"
#include <frg/manual_box.hpp>
#include <kernel/sched.hpp>
#include <kernel/task.hpp>
#include <kernel/wait.hpp>
#include <vm/heap.hpp>
#include <vm/vm.hpp>

namespace Gaia {

static pid_t current_pid = 0;

static List<Thread, &Thread::link> runq;
static List<Thread, &Thread::link> to_die;
static frg::manual_box<frg::simple_spinlock> sched_lock;
static frg::manual_box<frg::simple_spinlock> reaper_lock;
static Thread *current_thread = nullptr;
static Task *kernel_task = nullptr;
static bool restore_frame = false;
static Thread *idle_thread = nullptr;

pid_t sched_allocate_pid() { return current_pid++; }

// AAA this sucks so much
static Cpu *cpu = nullptr;

void sched_register_cpu(Cpu *_cpu) {
  cpu = _cpu;
  current_thread = cpu->current_thread;
}

Result<Thread *, Error> sched_new_thread(frg::string_view name, Task *task,
                                         Hal::CpuContext ctx, bool insert) {

  // FIXME: maybe use smart pointers?
  auto thread = new (Vm::Subsystem::SCHED) Thread();

  if (!thread)
    return Err(Error::OUT_OF_MEMORY);

  if (!task)
    return Err(Error::INVALID_PARAMETERS);

  thread->name = Vm::String(name);

  thread->task = task;
  thread->ctx = ctx;
  thread->state = Thread::RUNNING;
  thread->cpu = cpu;

  task->threads.push(thread);

  if (insert) {
    runq.insert_tail(thread);
  }

  return Ok(thread);
}

Task *sched_kernel_task() { return kernel_task; }

Result<Task *, Error> sched_new_task(pid_t pid, Task *parent, bool user) {

  auto task = new (Vm::Subsystem::SCHED) Task();

  task->cwd = Fs::root_vnode;
  task->pid = pid;

  task->parent = parent;

  if (parent)
    parent->children.insert_tail(task);

  if (pid == 0) {
    auto fd = new (Vm::Subsystem::FS)
        Posix::Fd(Posix::Fd::open("/dev/tty", O_RDWR).unwrap());
    task->fds.allocate(fd);
    task->fds.allocate(fd);
    task->fds.allocate(fd);
  }

  if (user) {
    task->space = new (Vm::Subsystem::SCHED) Vm::Space("task space", user);
    if (!task->space)
      return Err(Error::OUT_OF_MEMORY);
  } else {
    task->space = Vm::kernel_space;
  }

  return Ok(task);
}

Thread::~Thread() {
  Vm::vm_kernel_free(
      (void *)(ctx.info.syscall_kernel_stack - KERNEL_STACK_SIZE),
      KERNEL_STACK_SIZE / Hal::PAGE_SIZE);
  Vm::phys_free((void *)(Hal::virt_to_phys((uintptr_t)ctx.fpu_regs)));
}

Task::~Task() {
  for (auto thread : threads) {
    sched_send_to_death(thread);
  }

  for (auto fd : fds.data()) {
    if (fd) {
      delete fd;
    }
  }

  space->release();
  delete space;
}

static Thread *get_next_thread() {
  if (!runq.head()) {
    return idle_thread;
  }

  return runq.remove_head().unwrap();
}

Cpu *cpu_self() { return Hal::get_current_thread()->cpu; }

void sched_tick(Hal::InterruptFrame *frame) {
  ASSERT(cpu_self()->magic == 0xCAFEBABE);

  sched_lock->lock();

  if (restore_frame) {
    current_thread->ctx.save(frame);
  } else {
    restore_frame = true;
  }

  if (current_thread->state == Thread::RUNNING &&
      current_thread != idle_thread) {
    runq.insert_tail(current_thread);
  }

  current_thread = get_next_thread();
  current_thread->state = Thread::RUNNING;

  sched_lock->unlock();

  Hal::set_current_thread(current_thread);
  current_thread->task->space->activate();
  Hal::do_context_switch();
}

void sched_dequeue_and_die() {
  sched_dequeue_thread(sched_curr());
  sched_yield();
  Hal::halt();
}

void sched_yield() {
  Hal::disable_interrupts();
  Amd64::lapic_send_ipi(32);
  Hal::enable_interrupts();
}

void sched_suspend_thread(Thread *thread) {
  thread->state = Thread::SUSPENDED;
  sched_yield();
}

void sched_send_to_death(Thread *thread) {
  reaper_lock->lock();
  thread->state = Thread::EXITED;
  to_die.insert_tail(thread);
  reaper_lock->unlock();
}

void idle_thread_fn() { Hal::halt(); }

void reaper() {
  while (true) {
    if (to_die.length() > 0) {
      reaper_lock->lock();
      for (auto thread : to_die) {
        to_die.remove(thread);

        delete thread;
      }
      reaper_lock->unlock();
    } else {
      sched_yield();
    }
  }
}

Result<Void, Error> sched_init() {
  kernel_task = TRY(sched_new_task(-1, nullptr, false));

  sched_lock.initialize();
  reaper_lock.initialize();

  TRY(sched_new_worker_thread("reaper", (uintptr_t)reaper));

  current_thread = cpu->idle_thread =
      sched_new_worker_thread("idle thread", (uintptr_t)idle_thread_fn, false)
          .unwrap();
  cpu->idle_thread->cpu = cpu;
  cpu->current_thread = cpu->idle_thread;
  cpu->magic = 0xCAFEBABE;
  Hal::set_current_thread(cpu->idle_thread);

  return Ok({});
}

Thread *sched_curr() { return current_thread; }

Result<Thread *, Error> sched_new_worker_thread(frg::string_view name,
                                                uintptr_t entry_point,
                                                bool insert) {
  auto stack = (uintptr_t)Vm::vm_kernel_alloc(4);

  auto ctx = Hal::CpuContext(entry_point, stack + 0x4000, 0, false);
  return sched_new_thread(name, kernel_task, ctx, insert);
}

void sched_enqueue_thread(Thread *thread) { runq.insert_tail(thread); }
void sched_dequeue_thread(Thread *thread) { runq.remove(thread); }

void sched_wake_thread(Thread *thread) {
  thread->state = Thread::RUNNING;
  sched_enqueue_thread(thread);
}

} // namespace Gaia
