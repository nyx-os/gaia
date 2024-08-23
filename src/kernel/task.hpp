// @license:bsd2
#pragma once
#include <kernel/sched.hpp>
#include <kernel/wait.hpp>

namespace Gaia {

struct Task : public Waitable {
  pid_t pid;

  ListNode<Task> link;

  Vm::Vector<Thread *> threads;
  List<Task, &Task::link> children;

  Fs::Vnode *cwd;

  Posix::Fds fds;

  Task *parent;
  Vm::Space *space;

  int exit_code;
  bool has_exited = false;

  ~Task();
};

} // namespace Gaia