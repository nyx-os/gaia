/* SPDX-License-Identifier: BSD-2-Clause */
#include "amd64/asm.hpp"
#include "fs/vfs.hpp"
#include "hal/hal.hpp"
#include "kernel/elf.hpp"
#include "posix/fd.hpp"
#include "vm/heap.hpp"
#include "vm/phys.hpp"
#include "vm/vm_kernel.hpp"
#include <hal/mmu.hpp>
#include <kernel/sched.hpp>
#include <kernel/syscalls.hpp>
#include <kernel/wait.hpp>
#include <lib/log.hpp>
#define HAVE_ARCH_STRUCT_FLOCK
#include <kernel/task.hpp>
#include <linux/fcntl.h>
#include <posix/errno.hpp>
#include <sys/syscall.h>
#include <unistd.h>

#define TRACE 0

namespace Gaia {

using SysFunc = uint64_t(SyscallParams);

struct StraceDef {
  frg::string_view name;
  int param_count;
};

StraceDef get_strace_def(uint64_t num) {
  switch (num) {
  case SYS_read:
    return {"read", 3};
  case SYS_write:
    return {"write", 3};
  case SYS_mmap:
    return {"mmap", 6};
  case SYS_munmap:
    return {"munmap", 2};
  case SYS_lseek:
    return {"lseek", 3};
  case SYS_openat:
    return {"openat", 3};
  case SYS_execve:
    return {"execve", 3};
  case SYS_ioctl:
    return {"ioctl", 3};
  case SYS_close:
    return {"close", 1};
  case SYS_clone:
    return {"clone", 2};
  case SYS_wait4:
    return {"wait4", 4};
  case SYS_getpid:
    return {"getpid", 0};
  case SYS_getppid:
    return {"getppid", 0};
  case SYS_getcwd:
    return {"getcwd", 2};
  case SYS_getdents64:
    return {"getdents64", 3};
  case SYS_newfstatat:
    return {"newfstatat", 4};
  case SYS_exit:
  case SYS_exit_group:
    return {"exit_group", 1};
  case SYS_fcntl:
    return {"fcntl", 3};
  case SYS_dup3:
    return {"dup3", 3};
  default:
    error("Unimplemented strace for {}", num);
    break;
  }

  return {"invalid", 0};
}

uint8_t get_random_color(pid_t seed) {
  if (seed & 1) {
    return (uint8_t)(((seed >> 1) % 7) + 1);
  } else {
    return (uint8_t)((((seed >> 1) % 7) + 1) + 60);
  }
}

template <typename... T>
void thread_log(Thread *thread, const char *str, T... args) {
  logger() << frg::fmt("\033[{}m[{}]({}) \033[0m",
                       30 + get_random_color(thread->task->pid),
                       thread->name.size() == 0 ? "" : thread->name,
                       thread->task->pid)
           << frg::fmt(str, args...) << "\n"
           << frg::endlog;
}

#define LOG_CURR(...) (thread_log(sched_curr(), __VA_ARGS__))

uint64_t trace(uint64_t ret, uint64_t num, SyscallParams params) {
  auto def = get_strace_def(num);

  switch (def.param_count) {
  case 0:
    LOG_CURR("{}() -> {:x} ({})", def.name, ret, (int64_t)ret);
    break;
  case 1:
    LOG_CURR("{}({:x}) -> {:x} ({})", def.name, params.param1, ret,
             (int64_t)ret);
    break;
  case 2:
    LOG_CURR("{}({:x}, {:x}) -> {:x} ({})", def.name, params.param1,
             params.param2, ret, (int64_t)ret);
    break;
  case 3:
    LOG_CURR("{}({:x}, {:x}, {:x}) -> {:x} ({})", def.name, params.param1,
             params.param2, params.param3, ret, (int64_t)ret);
    break;
  case 4:
    LOG_CURR("{}({:x}, {:x}, {:x}, {:x}) -> {:x} ({})", def.name, params.param1,
             params.param2, params.param3, params.param4, ret, (int64_t)ret);
    break;

  case 5:
    LOG_CURR("{}({:x}, {:x}, {:x}, {:x}, {:x}) -> {:x} ({})", def.name,
             params.param1, params.param2, params.param3, params.param4,
             params.param5, ret, (int64_t)ret);
    break;
  case 6:
    LOG_CURR("{}({:x}, {:x}, {:x}, {:x}, {:x}, {:x}) -> {:x} ({})", def.name,
             params.param1, params.param2, params.param3, params.param4,
             params.param5, params.param6, ret, (int64_t)ret);
    break;
  }
  return ret;
}

uint64_t sys_read(SyscallParams params) {
  auto fdnum = params.param1;
  auto buf = (void *)params.param2;
  auto count = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto read_ret = fd.value()->read(buf, count);

  if (read_ret.is_err()) {
    return -(read_ret.error().value());
  }

  return read_ret.value().value();
}

uint64_t sys_write(SyscallParams params) {
  auto fdnum = params.param1;
  auto buf = (void *)params.param2;
  auto count = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto ret = fd.value()->write(buf, count);

  if (ret.is_err()) {
    return -(ret.error().value());
  }

  return ret.unwrap();
}

uint64_t sys_ioctl(SyscallParams params) {
  auto fdnum = params.param1;
  auto req = params.param2;
  auto arg = (void *)params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto ret = fd.value()->ioctl(req, arg);

  if (ret.is_err()) {
    return -(ret.error().value());
  }

  return ret.unwrap();
}

uint64_t sys_openat(SyscallParams params) {
  int dirfd = params.param1;
  const char *pathname = (const char *)params.param2;
  int flags = (int)params.param3;

  if (dirfd != AT_FDCWD) {
    error("openat: TODO: add support for dirfd != AT_FDCWD");
    return -EBADF;
  }

  auto fd = Posix::Fd::open(pathname, flags);

  if (fd.is_err()) {
    return -(fd.error().value());
  }

  auto ret = sched_curr()->task->fds.allocate(new Posix::Fd{fd.unwrap()});

  if (!ret.has_value()) {
    return -EBADF;
  }

  return ret.value();
}

uint64_t sys_lseek(SyscallParams params) {
  int fdnum = params.param1;
  off_t offset = params.param2;
  auto whence = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto convert_whence = [](uint64_t whence) -> Stream::Whence {
    switch (whence) {
    case SEEK_END:
      return Stream::Whence::END;
    case SEEK_SET:
      return Stream::Whence::SET;
    case SEEK_CUR:
      return Stream::Whence::CURRENT;
    }

    return (Stream::Whence)-1;
  };

  auto ret = fd.value()->seek(offset, convert_whence(whence));

  if (ret.is_err()) {
    return -(ret.error().value());
  }

  return ret.unwrap();
}

uint64_t sys_close(SyscallParams params) {

  auto &fds = sched_curr()->task->fds;
  auto fdnum = params.param1;

  if (!fds.get(fdnum)) {
    return -EBADF;
  }
  fds.free(fdnum);
  return 0;
}

uint64_t sys_getpid(SyscallParams params) {
  (void)params;
  return sched_curr()->task->pid;
}

uint64_t sys_getppid(SyscallParams params) {
  (void)params;
  return sched_curr()->task->parent->pid;
}

uint64_t sys_getcwd(SyscallParams params) {
  auto buf = (char *)params.param1;
  auto size = params.param2;

  auto cwd = Fs::vfs_get_absolute_path(sched_curr()->task->cwd);

  if (size > cwd.size())
    return -ERANGE;

  memcpy(buf, cwd.data(), size);

  return (uint64_t)buf;
}

uint64_t sys_exit_group(SyscallParams params) {

  auto curr_thread = sched_curr();

  curr_thread->state = Thread::EXITED;
  curr_thread->task->has_exited = true;
  curr_thread->task->exit_code = (int)params.param1;

  curr_thread->task->parent->trigger_event().unwrap();

  Hal::disable_interrupts();
  sched_send_to_death(curr_thread);
  sched_dequeue_and_die();
  Hal::enable_interrupts();

  return 0;
}

uint64_t sys_execve(SyscallParams params) {
  auto filename = (const char *)params.param1;
  auto argv = (char **)params.param2;
  auto envp = (char **)params.param3;

  // FIXME: maybe avoid looking up the node twice?
  auto lookup_ret = Fs::vfs_find(filename);

  if (lookup_ret.is_err()) {
    return -(Posix::error_to_errno(lookup_ret.error().value()));
  }

  Vm::Vector<const char *> sanitized_argv;
  Vm::Vector<const char *> sanitized_envp;

  auto task = sched_curr()->task;

  for (char **arg = argv; *arg; arg++) {
    auto new_str = (char *)Vm::malloc(strlen(*arg) + 1);
    memcpy(new_str, *arg, strlen(*arg) + 1);
    sanitized_argv.push(new_str);
  }

  sanitized_argv.push(nullptr);

  for (char **env = envp; *env; env++) {
    auto new_str = (char *)Vm::malloc(strlen(*env) + 1);
    memcpy(new_str, *env, strlen(*env) + 1);
    sanitized_envp.push(new_str);
  }
  sanitized_envp.push(nullptr);

  auto prev_space = task->space;

  task->space = new Vm::Space("task space", true);

  for (auto thread : task->threads) {
    sched_send_to_death(thread);
  }

  task->threads.clear();

  auto exec_ret =
      execve(*task, filename, sanitized_argv.data(), sanitized_envp.data());

  if (exec_ret.is_err()) {
    return -(Posix::error_to_errno(exec_ret.error().value()));
  }

  for (auto arg : sanitized_argv) {
    if (arg) {
      Vm::free((void *)arg);
    }
  }

  for (auto env : sanitized_envp) {
    if (env) {
      Vm::free((void *)env);
    }
  }

  prev_space->release();
  delete prev_space;

  sched_yield();

  Hal::halt();

  return 0;
}

uint64_t sys_clone(SyscallParams params) {
  // TODO: actually not ignore the parameters
  (void)params;

  auto curr_task = sched_curr()->task;

  auto new_task =
      sched_new_task(sched_allocate_pid(), curr_task, true).unwrap();

  curr_task->space->copy(new_task->space);

  size_t i = 0;
  for (auto fd : curr_task->fds.data()) {
    if (fd)
      new_task->fds.set(i, new Posix::Fd(*fd));
    i++;
  }

  auto new_ctx = sched_curr()->ctx;

  new_ctx.info.syscall_kernel_stack =
      (uintptr_t)Vm::vm_kernel_alloc(KERNEL_STACK_SIZE / Hal::PAGE_SIZE) +
      KERNEL_STACK_SIZE;

  new_ctx.fpu_regs =
      (void *)Hal::phys_to_virt((uintptr_t)Vm::phys_alloc(true).unwrap());

  new_ctx.regs = *params.frame;
  new_ctx.regs.rax = 0;

  sched_new_thread("forked thread", new_task, new_ctx, true);

  return new_task->pid;
}

// TODO: add support for rusage
uint64_t sys_wait4(SyscallParams params) {
  pid_t upid = params.param1;
  int *status = (int *)params.param2;
  int flags = params.param3;

  (void)flags;

  auto curr_task = sched_curr()->task;

  while (true) {
    for (auto child : curr_task->children) {
      if (child->has_exited) {
        if (upid != -1 && child->pid != upid)
          continue;

        curr_task->children.remove(child);
        *status = child->exit_code;
        return child->pid;
      }
    }

    curr_task->await_event(-1).unwrap();
  }

  ASSERT(false);

  return -1;
}

uint64_t sys_newfstatat(SyscallParams params) {
  int dirfd = params.param1;
  const char *pathname = (const char *)params.param2;
  struct stat *statbuf = (struct stat *)params.param3;
  auto flags = params.param4;

  Posix::Fd file{};

  if (dirfd != AT_FDCWD && !(flags & AT_EMPTY_PATH)) {
    auto dir = sched_curr()->task->fds.get(dirfd);

    if (!dir.has_value()) {
      return -EBADF;
    }

    auto fd = Posix::Fd::openat(*dir.value(), pathname, O_RDONLY);

    if (fd.is_err()) {
      return -(fd.error().value());
    }

    auto file = fd.unwrap();
    auto stat = file.stat();

    if (stat.is_err()) {
      return -(stat.error().value());
    }

    *statbuf = stat.unwrap();
  }

  // Linux-specific: interpret dirfd as a file to do stat on instead
  else if (flags & AT_EMPTY_PATH) {

    auto fd = sched_curr()->task->fds.get(dirfd);

    if (!fd.has_value()) {
      return -EBADF;
    }

    auto stat = fd.value()->stat();

    if (stat.is_err()) {
      return -(stat.error().value());
    }

    *statbuf = stat.unwrap();
  }

  else {
    auto fd = Posix::Fd::open(pathname, O_RDONLY);

    if (fd.is_err()) {
      return -(fd.error().value());
    }

    auto file = fd.unwrap();
    auto stat = file.stat();

    if (stat.is_err()) {
      return -(stat.error().value());
    }

    *statbuf = stat.unwrap();
  }

  return 0;
}

uint64_t sys_fcntl(SyscallParams params) {
  auto fdnum = (int)params.param1;
  auto cmd = params.param2;
  auto arg = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  switch (cmd) {
  case F_DUPFD: {
    auto new_fd = new Posix::Fd(*fd.value());
    auto newfd =
        sched_curr()->task->fds.allocate_greater_than_or_equal(arg, new_fd);

    if (!newfd.has_value()) {
      return -EINVAL;
    }

    return newfd.value();
  }

  case F_GETFL: {
    return fd.value()->get_status();
  }

  case F_SETFL: {
    fd.value()->set_status((int)arg);
    return 0;
  }

  case F_SETFD: {
    fd.value()->set_flags((int)arg);
    return 0;
  }

  case F_GETFD: {
    return fd.value()->get_flags();
  }
  }

  error("fcntl: unknown command {}", cmd);

  return -EINVAL;
}

uint64_t sys_dup3(SyscallParams params) {
  auto fdnum = (int)params.param1;
  auto newfdnum = params.param2;
  auto flags = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto new_fd = new Posix::Fd(*fd.value());
  auto newfd = sched_curr()->task->fds.set(newfdnum, new_fd);

  if (flags & O_CLOEXEC)
    new_fd->set_flags(FD_CLOEXEC);

  if (newfd.is_err()) {
    return -(newfd.error().value());
  }

  return newfdnum;
}

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20

// FIXME: this code sucks
// TODO: move this to another file
uint64_t sys_mmap(SyscallParams params) {
  auto hint = params.param1;
  auto size = params.param2;
  auto req_prot = params.param3;
  auto flags = params.param4;
  // auto fd = params.param5;
  // auto off = params.param6;

  int prot{};
  size_t actual_flags = flags & 0xFFFFFFFF;

  if (req_prot & PROT_READ)
    prot |= Hal::Vm::Prot::READ;
  if (req_prot & PROT_WRITE)
    prot |= Hal::Vm::Prot::WRITE;
  if (req_prot & PROT_EXEC)
    prot |= Hal::Vm::Prot::EXECUTE;

  if (actual_flags & MAP_FIXED) {
    return sched_curr()
        ->task->space->new_anon(hint, size, (Hal::Vm::Prot)prot)
        .unwrap();
  } else if (actual_flags & MAP_ANONYMOUS) {
    return sched_curr()
        ->task->space->new_anon(frg::null_opt, size, (Hal::Vm::Prot)prot)
        .unwrap();
  }

  return -EINVAL;
}

uint64_t sys_munmap(SyscallParams params) {
  auto addr = params.param1;
  auto size = params.param2;
  auto ret = sched_curr()->task->space->unmap(addr, size);

  if (ret.is_err()) {
    return -Posix::error_to_errno(ret.error().value());
  }

  return 0;
}

uint64_t sys_getdents64(SyscallParams params) {
  auto fdnum = params.param1;
  void *buf = (void *)params.param2;
  auto max_count = params.param3;

  auto fd = sched_curr()->task->fds.get(fdnum);

  if (!fd.has_value()) {
    return -EBADF;
  }

  auto ret = fd.value()->readdir(buf, max_count);

  if (ret.is_err()) {
    return -ret.error().value();
  }

  return ret.value().value();
}

#if TRACE
#define DO_TRACE(x) trace((x), num, params)
#else
#define DO_TRACE(x) (x)
#endif

uint64_t syscall(int num, SyscallParams params) {
  switch (num) {
  case SYS_read:
    return DO_TRACE(sys_read(params));
  case SYS_write:
    return DO_TRACE(sys_write(params));
  case SYS_close:
    return DO_TRACE(sys_close(params));
  case SYS_lseek:
    return DO_TRACE(sys_lseek(params));
  case SYS_mmap:
    return DO_TRACE(sys_mmap(params));
  case SYS_openat:
    return DO_TRACE(sys_openat(params));
  case SYS_ioctl:
    return DO_TRACE(sys_ioctl(params));
  case SYS_exit:
  case SYS_exit_group:
    return DO_TRACE(sys_exit_group(params));
  case SYS_clone:
    return DO_TRACE(sys_clone(params));
  case SYS_execve:
    return DO_TRACE(sys_execve(params));
  case SYS_wait4:
    return DO_TRACE(sys_wait4(params));
  case SYS_getpid:
    return DO_TRACE(sys_getpid(params));
  case SYS_getppid:
    return DO_TRACE(sys_getppid(params));
  case SYS_getcwd:
    return DO_TRACE(sys_getcwd(params));
  case SYS_newfstatat:
    return DO_TRACE(sys_newfstatat(params));
  case SYS_fcntl:
    return DO_TRACE(sys_fcntl(params));
  case SYS_dup3:
    return DO_TRACE(sys_dup3(params));
  case SYS_munmap:
    return DO_TRACE(sys_munmap(params));
  case SYS_getdents64:
    return DO_TRACE(sys_getdents64(params));
  case SYS_faccessat:
    return 0;
  case SYS_fadvise64:
    return -ENOSYS;

  case SYS_arch_prctl:
    if (params.param1 == 0x1002) {
      Amd64::wrmsr(0xc0000100, (uint64_t)params.param2);
      return 0;
    }
    return -ENOSYS;

  default:
    error("Unknown syscall number {}", num);
    return -ENOSYS;
  }
}

} // namespace Gaia
